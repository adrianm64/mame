// license:BSD-3-Clause
// copyright-holders:Phill Harvey-Smith, Carl
/*
    drivers/rmnimbus.c

    Research machines Nimbus.

    2009-11-14, P.Harvey-Smith.

*/

#include "includes/rmnimbus.h"
#include "cpu/mcs51/mcs51.h"
#include "imagedev/flopdrv.h"
#include "formats/pc_dsk.h"
#include "bus/scsi/scsihd.h"
#include "bus/scsi/s1410.h"
#include "bus/scsi/acb4070.h"
#include "bus/isa/fdc.h"
#include "bus/rs232/rs232.h"
#include "machine/rmnkbd.h"
#include "softlist.h"

static SLOT_INTERFACE_START(rmnimbus_floppies)
	SLOT_INTERFACE( "35dd", FLOPPY_35_DD )
SLOT_INTERFACE_END

static SLOT_INTERFACE_START(keyboard)
	SLOT_INTERFACE("rmnkbd", RMNIMBUS_KEYBOARD)
SLOT_INTERFACE_END

static ADDRESS_MAP_START(nimbus_mem, AS_PROGRAM, 16, rmnimbus_state )
	AM_RANGE( 0x00000, 0x1FFFF ) AM_RAMBANK(RAM_BANK00_TAG)
	AM_RANGE( 0x20000, 0x3FFFF ) AM_RAMBANK(RAM_BANK01_TAG)
	AM_RANGE( 0x40000, 0x5FFFF ) AM_RAMBANK(RAM_BANK02_TAG)
	AM_RANGE( 0x60000, 0x7FFFF ) AM_RAMBANK(RAM_BANK03_TAG)
	AM_RANGE( 0x80000, 0x9FFFF ) AM_RAMBANK(RAM_BANK04_TAG)
	AM_RANGE( 0xA0000, 0xBFFFF ) AM_RAMBANK(RAM_BANK05_TAG)
	AM_RANGE( 0xC0000, 0xDFFFF ) AM_RAMBANK(RAM_BANK06_TAG)
	AM_RANGE( 0xE0000, 0xEFFFF ) AM_RAMBANK(RAM_BANK07_TAG)
	AM_RANGE( 0xF0000, 0xFFFFF ) AM_ROM AM_REGION(MAINCPU_TAG, 0x0f0000)
ADDRESS_MAP_END

static ADDRESS_MAP_START(nimbus_io, AS_IO, 16, rmnimbus_state )
	AM_RANGE( 0x0000, 0x0031) AM_READWRITE(nimbus_video_io_r, nimbus_video_io_w)
	AM_RANGE( 0x0080, 0x0081) AM_READWRITE8(nimbus_mcu_r, nimbus_mcu_w, 0x00FF)
	AM_RANGE( 0x0092, 0x0093) AM_READWRITE8(nimbus_iou_r, nimbus_iou_w, 0x00FF)
	AM_RANGE( 0x00A4, 0x00A5) AM_READWRITE8(nimbus_mouse_js_r, nimbus_mouse_js_w, 0x00FF)
	AM_RANGE( 0X00c0, 0X00cf) AM_READWRITE8(nimbus_pc8031_r, nimbus_pc8031_w, 0x00FF)
	AM_RANGE( 0X00e0, 0X00ef) AM_DEVREADWRITE8(AY8910_TAG, ay8910_device, data_r, address_data_w, 0x00FF)
	AM_RANGE( 0x00f0, 0x00f7) AM_DEVREADWRITE8(Z80SIO_TAG, z80sio2_device, cd_ba_r, cd_ba_w, 0x00ff)
	AM_RANGE( 0x0400, 0x0401) AM_WRITE8(fdc_ctl_w, 0x00ff)
	AM_RANGE( 0x0408, 0x040f) AM_DEVREADWRITE8(FDC_TAG, wd2793_t, read, write, 0x00ff)
	AM_RANGE( 0x0410, 0x041f) AM_READWRITE8(scsi_r, scsi_w, 0x00ff)
	AM_RANGE( 0x0480, 0x049f) AM_DEVREADWRITE8(VIA_TAG, via6522_device, read, write, 0x00FF)
ADDRESS_MAP_END


static INPUT_PORTS_START( nimbus )
	PORT_START("config")
	PORT_CONFNAME( 0x01, 0x00, "Input Port 0 Device")
	PORT_CONFSETTING( 0x00, "Mouse" )
	PORT_CONFSETTING( 0x01, DEF_STR( Joystick ) )

	PORT_START(JOYSTICK0_TAG)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY // XB
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY // XA
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY // YA
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY // YB
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START(MOUSE_BUTTON_TAG)  /* Mouse buttons */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Mouse Button 2") PORT_CODE(MOUSECODE_BUTTON2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button 1") PORT_CODE(MOUSECODE_BUTTON1)

	PORT_START(MOUSEX_TAG) /* Mouse - X AXIS */
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START(MOUSEY_TAG) /* Mouse - Y AXIS */
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

INPUT_PORTS_END

static ADDRESS_MAP_START(nimbus_iocpu_mem, AS_PROGRAM, 8, rmnimbus_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( nimbus_iocpu_io , AS_IO, 8, rmnimbus_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x000FF) AM_READWRITE(nimbus_pc8031_iou_r, nimbus_pc8031_iou_w)
	AM_RANGE(0x20000, 0x20004) AM_READWRITE(nimbus_pc8031_port_r, nimbus_pc8031_port_w)
ADDRESS_MAP_END

static const UINT16 def_config[16] =
{
	0x0280, 0x017F, 0xE824, 0x8129,
	0x0329, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x8893, 0x2025, 0xB9E6
};

static MACHINE_CONFIG_START( nimbus, rmnimbus_state )
	/* basic machine hardware */
	MCFG_CPU_ADD(MAINCPU_TAG, I80186, 16000000) // the cpu is a 10Mhz part but the serial clocks are wrong unless it runs at 8Mhz
	MCFG_CPU_PROGRAM_MAP(nimbus_mem)
	MCFG_CPU_IO_MAP(nimbus_io)
	MCFG_80186_IRQ_SLAVE_ACK(READ8(rmnimbus_state, cascade_callback))
	MCFG_80186_TMROUT0_HANDLER(DEVWRITELINE(Z80SIO_TAG, z80dart_device, rxca_w))
	MCFG_80186_TMROUT1_HANDLER(DEVWRITELINE(Z80SIO_TAG, z80dart_device, rxtxcb_w))

	MCFG_CPU_ADD(IOCPU_TAG, I8031, 11059200)
	MCFG_CPU_PROGRAM_MAP(nimbus_iocpu_mem)
	MCFG_CPU_IO_MAP(nimbus_iocpu_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_RAW_PARAMS( XTAL_4_433619MHz*2,650,0,640,260,0,250)
	MCFG_SCREEN_UPDATE_DRIVER(rmnimbus_state, screen_update_nimbus)
	//MCFG_SCREEN_VIDEO_ATTRIBUTES(VIDEO_UPDATE_SCANLINE)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD("palette", 16)

	/* Backing storage */
	MCFG_WD2793_ADD(FDC_TAG, 1000000)
	MCFG_WD_FDC_FORCE_READY
	MCFG_WD_FDC_INTRQ_CALLBACK(WRITELINE(rmnimbus_state,nimbus_fdc_intrq_w))
	MCFG_WD_FDC_DRQ_CALLBACK(WRITELINE(rmnimbus_state,nimbus_fdc_drq_w))
	MCFG_FLOPPY_DRIVE_ADD(FDC_TAG":0", rmnimbus_floppies, "35dd", isa8_fdc_device::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD(FDC_TAG":1", rmnimbus_floppies, "35dd", isa8_fdc_device::floppy_formats)

	MCFG_DEVICE_ADD(SCSIBUS_TAG, SCSI_PORT, 0)
	MCFG_SCSI_DATA_INPUT_BUFFER("scsi_data_in")
	MCFG_SCSI_MSG_HANDLER(WRITELINE(rmnimbus_state, write_scsi_msg))
	MCFG_SCSI_BSY_HANDLER(WRITELINE(rmnimbus_state, write_scsi_bsy))
	MCFG_SCSI_IO_HANDLER(WRITELINE(rmnimbus_state, write_scsi_io))
	MCFG_SCSI_CD_HANDLER(WRITELINE(rmnimbus_state, write_scsi_cd))
	MCFG_SCSI_REQ_HANDLER(WRITELINE(rmnimbus_state, write_scsi_req))

	MCFG_SCSIDEV_ADD(SCSIBUS_TAG ":" SCSI_PORT_DEVICE1, "harddisk", SCSIHD, SCSI_ID_0)
	MCFG_SCSIDEV_ADD(SCSIBUS_TAG ":" SCSI_PORT_DEVICE2, "harddisk", SCSIHD, SCSI_ID_1)
	MCFG_SCSIDEV_ADD(SCSIBUS_TAG ":" SCSI_PORT_DEVICE3, "harddisk", ACB4070, SCSI_ID_2)
	MCFG_SCSIDEV_ADD(SCSIBUS_TAG ":" SCSI_PORT_DEVICE4, "harddisk", S1410, SCSI_ID_3)

	MCFG_SCSI_OUTPUT_LATCH_ADD("scsi_data_out", SCSIBUS_TAG)
	MCFG_DEVICE_ADD("scsi_data_in", INPUT_BUFFER, 0)

	MCFG_DEVICE_ADD("scsi_ctrl_out", OUTPUT_LATCH, 0)
	MCFG_OUTPUT_LATCH_BIT0_HANDLER(DEVWRITELINE(SCSIBUS_TAG, SCSI_PORT_DEVICE, write_rst))
	MCFG_OUTPUT_LATCH_BIT1_HANDLER(DEVWRITELINE(SCSIBUS_TAG, SCSI_PORT_DEVICE, write_sel))
	MCFG_OUTPUT_LATCH_BIT2_HANDLER(WRITELINE(rmnimbus_state, write_scsi_iena))

	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1536K")
	MCFG_RAM_EXTRA_OPTIONS("128K,256K,384K,512K,640K,1024K")

	/* Peripheral chips */
	MCFG_Z80SIO2_ADD(Z80SIO_TAG, 4000000, 0, 0, 0, 0)
	MCFG_Z80DART_OUT_TXDB_CB(DEVWRITELINE("rs232b", rs232_port_device, write_txd))
	MCFG_Z80DART_OUT_DTRB_CB(DEVWRITELINE("rs232b", rs232_port_device, write_dtr))
	MCFG_Z80DART_OUT_RTSB_CB(DEVWRITELINE("rs232b", rs232_port_device, write_rts))
	MCFG_Z80DART_OUT_INT_CB(WRITELINE(rmnimbus_state, sio_interrupt))

	MCFG_RS232_PORT_ADD("rs232a", keyboard, "rmnkbd")
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE(Z80SIO_TAG, z80dart_device, rxa_w))

	MCFG_RS232_PORT_ADD("rs232b", default_rs232_devices, NULL)
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE(Z80SIO_TAG, z80dart_device, rxb_w))
	MCFG_RS232_DCD_HANDLER(DEVWRITELINE(Z80SIO_TAG, z80dart_device, dcdb_w))
	MCFG_RS232_RI_HANDLER(DEVWRITELINE(Z80SIO_TAG, z80dart_device, rib_w))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE(Z80SIO_TAG, z80dart_device, ctsb_w))

	MCFG_EEPROM_SERIAL_93C06_ADD(ER59256_TAG)
	MCFG_EEPROM_DATA(def_config,sizeof(def_config))

	MCFG_DEVICE_ADD(VIA_TAG, VIA6522, 1000000)
	MCFG_VIA6522_WRITEPA_HANDLER(DEVWRITE8("cent_data_out", output_latch_device, write))
	MCFG_VIA6522_WRITEPB_HANDLER(WRITE8(rmnimbus_state,nimbus_via_write_portb))
	MCFG_VIA6522_CA2_HANDLER(DEVWRITELINE(CENTRONICS_TAG, centronics_device, write_strobe))
	MCFG_VIA6522_IRQ_HANDLER(DEVWRITELINE(MAINCPU_TAG, i80186_cpu_device, int3_w))

	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, centronics_devices, "printer")
	MCFG_CENTRONICS_ACK_HANDLER(DEVWRITELINE(VIA_TAG, via6522_device, write_ca1)) MCFG_DEVCB_INVERT

	MCFG_CENTRONICS_OUTPUT_LATCH_ADD("cent_data_out", "centronics")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO(MONO_TAG)
	MCFG_SOUND_ADD(AY8910_TAG, AY8910, 2000000)
	MCFG_AY8910_PORT_A_WRITE_CB(WRITE8(rmnimbus_state, nimbus_sound_ay8910_porta_w))
	MCFG_AY8910_PORT_B_WRITE_CB(WRITE8(rmnimbus_state, nimbus_sound_ay8910_portb_w))
	MCFG_SOUND_ROUTE(ALL_OUTPUTS,MONO_TAG, 0.75)

	MCFG_SOUND_ADD(MSM5205_TAG, MSM5205, 384000)
	MCFG_MSM5205_VCLK_CB(WRITELINE(rmnimbus_state, nimbus_msm5205_vck)) /* VCK function */
	MCFG_MSM5205_PRESCALER_SELECTOR(MSM5205_S48_4B)      /* 8 kHz */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, MONO_TAG, 0.75)

	/* Software list */
	MCFG_SOFTWARE_LIST_ADD("disk_list","nimbus")
MACHINE_CONFIG_END


ROM_START( nimbus )
	ROM_REGION( 0x100000, MAINCPU_TAG, 0 )

	ROM_SYSTEM_BIOS(0, "v131a", "Nimbus BIOS v1.31a (1986-06-18)")
	ROMX_LOAD("sys1-1.31a-16128-1986-06-18.rom", 0xf0001, 0x8000, CRC(6416eb05) SHA1(1b640163a7efbc24381c7b24976a8609c066959b),ROM_SKIP(1) | ROM_BIOS(1)  )
	ROMX_LOAD("sys2-1.31a-16129-1986-06-18.rom", 0xf0000, 0x8000, CRC(b224359d) SHA1(456bbe37afcd4429cca76ba2d6bd534dfda3fc9c),ROM_SKIP(1) | ROM_BIOS(1)  )

	ROM_SYSTEM_BIOS(1, "v132f", "Nimbus BIOS v1.32f (1989-10-20)")
	ROMX_LOAD("sys-1-1.32f-22779-1989-10-20.rom", 0xf0001, 0x8000, CRC(786c31e8) SHA1(da7f828f7f96087518bea1a3d89fee59b283b4ba),ROM_SKIP(1) | ROM_BIOS(2) )
	ROMX_LOAD("sys-2-1.32f-22779-1989-10-20.rom", 0xf0000, 0x8000, CRC(0be3db64) SHA1(af806405ec6fbc20385705f90d5059a47de17b08),ROM_SKIP(1) | ROM_BIOS(2) )

	ROM_SYSTEM_BIOS(2, "v140d", "Nimbus BIOS v1.40d (1990-xx-xx)")
	ROMX_LOAD("sys-1-1.40d.rom", 0xf0001, 0x8000, CRC(b8d3dc0b) SHA1(82e0dcdc6c7a83339af68d6cb61211fcb14bed88),ROM_SKIP(1) | ROM_BIOS(3) )
	ROMX_LOAD("sys-2-1.40d.rom", 0xf0000, 0x8000, CRC(b0826b0b) SHA1(3baa369a0e7ef138ca29aae0ee8a89ab670a02b9),ROM_SKIP(1) | ROM_BIOS(3) )

	ROM_REGION( 0x4000, IOCPU_TAG, 0 )
	ROM_LOAD("hexec-v1.02u-13488-1985-10-29.rom", 0x0000, 0x1000, CRC(75c6adfd) SHA1(0f11e0b7386c6368d20e1fc7a6196d670f924825))
ROM_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE INPUT   INIT  COMPANY  FULLNAME   FLAGS */
COMP( 1986, nimbus,     0,      0,      nimbus, nimbus, driver_device, 0,   "Research Machines", "Nimbus", 0)
