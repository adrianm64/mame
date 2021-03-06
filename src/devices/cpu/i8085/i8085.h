// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller, hap

#ifndef __I8085_H__
#define __I8085_H__


/***************************************************************************
    CONSTANTS
***************************************************************************/

enum
{
	I8085_PC, I8085_SP, I8085_AF, I8085_BC, I8085_DE, I8085_HL,
	I8085_A, I8085_B, I8085_C, I8085_D, I8085_E, I8085_F, I8085_H, I8085_L,
	I8085_STATUS, I8085_SOD, I8085_SID, I8085_INTE,
	I8085_HALT, I8085_IM
};

#define I8085_INTR_LINE     0
#define I8085_RST55_LINE    1
#define I8085_RST65_LINE    2
#define I8085_RST75_LINE    3

#define I8085_STATUS_INTA   0x01
#define I8085_STATUS_WO     0x02
#define I8085_STATUS_STACK  0x04
#define I8085_STATUS_HLTA   0x08
#define I8085_STATUS_OUT    0x10
#define I8085_STATUS_M1     0x20
#define I8085_STATUS_INP    0x40
#define I8085_STATUS_MEMR   0x80


/* STATUS changed callback */
#define MCFG_I8085A_STATUS(_devcb) \
	devcb = &i8085a_cpu_device::set_out_status_func(*device, DEVCB_##_devcb);

/* INTE changed callback */
#define MCFG_I8085A_INTE(_devcb) \
	devcb = &i8085a_cpu_device::set_out_inte_func(*device, DEVCB_##_devcb);

/* SID changed callback (8085A only) */
#define MCFG_I8085A_SID(_devcb) \
	devcb = &i8085a_cpu_device::set_in_sid_func(*device, DEVCB_##_devcb);

/* SOD changed callback (8085A only) */
#define MCFG_I8085A_SOD(_devcb) \
	devcb = &i8085a_cpu_device::set_out_sod_func(*device, DEVCB_##_devcb);


class i8085a_cpu_device :  public cpu_device
{
public:
	// construction/destruction
	i8085a_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
	i8085a_cpu_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, uint32_t clock, const char *shortname, const char *source, int cputype);

	// static configuration helpers
	template<class _Object> static devcb_base &set_out_status_func(device_t &device, _Object object) { return downcast<i8085a_cpu_device &>(device).m_out_status_func.set_callback(object); }
	template<class _Object> static devcb_base &set_out_inte_func(device_t &device, _Object object) { return downcast<i8085a_cpu_device &>(device).m_out_inte_func.set_callback(object); }
	template<class _Object> static devcb_base &set_in_sid_func(device_t &device, _Object object) { return downcast<i8085a_cpu_device &>(device).m_in_sid_func.set_callback(object); }
	template<class _Object> static devcb_base &set_out_sod_func(device_t &device, _Object object) { return downcast<i8085a_cpu_device &>(device).m_out_sod_func.set_callback(object); }

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	// device_execute_interface overrides
	virtual uint32_t execute_min_cycles() const override { return 4; }
	virtual uint32_t execute_max_cycles() const override { return 16; }
	virtual uint32_t execute_input_lines() const override { return 4; }
	virtual uint32_t execute_default_irq_vector() const override { return 0xff; }
	virtual void execute_run() override;
	virtual void execute_set_input(int inputnum, int state) override;
	virtual uint64_t execute_clocks_to_cycles(uint64_t clocks) const override { return (clocks + 2 - 1) / 2; }
	virtual uint64_t execute_cycles_to_clocks(uint64_t cycles) const override { return (cycles * 2); }

	// device_memory_interface overrides
	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const override { return (spacenum == AS_PROGRAM) ? &m_program_config : ( (spacenum == AS_IO) ? &m_io_config : nullptr ); }

	// device_state_interface overrides
	virtual void state_string_export(const device_state_entry &entry, std::string &str) const override;
	virtual void state_export(const device_state_entry &entry) override;
	virtual void state_import(const device_state_entry &entry) override;

	// device_disasm_interface overrides
	virtual uint32_t disasm_min_opcode_bytes() const override { return 1; }
	virtual uint32_t disasm_max_opcode_bytes() const override { return 3; }
	virtual offs_t disasm_disassemble(std::ostream &stream, offs_t pc, const uint8_t *oprom, const uint8_t *opram, uint32_t options) override;

private:
	address_space_config m_program_config;
	address_space_config m_io_config;

	devcb_write8       m_out_status_func;
	devcb_write_line   m_out_inte_func;
	devcb_read_line    m_in_sid_func;
	devcb_write_line   m_out_sod_func;

	int                 m_cputype;        /* 0 8080, 1 8085A */
	PAIR                m_PC,m_SP,m_AF,m_BC,m_DE,m_HL,m_WZ;
	uint8_t               m_HALT;
	uint8_t               m_IM;             /* interrupt mask (8085A only) */
	uint8_t               m_STATUS;         /* status word */

	uint8_t               m_after_ei;       /* post-EI processing; starts at 2, check for ints at 0 */
	uint8_t               m_nmi_state;      /* raw NMI line state */
	uint8_t               m_irq_state[4];   /* raw IRQ line states */
	uint8_t               m_trap_pending;   /* TRAP interrupt latched? */
	uint8_t               m_trap_im_copy;   /* copy of IM register when TRAP was taken */
	uint8_t               m_sod_state;      /* state of the SOD line */

	bool                m_ietemp;         /* import/export temp space */

	address_space *m_program;
	direct_read_data *m_direct;
	address_space *m_io;
	int                 m_icount;

	/* cycles lookup */
	static const uint8_t lut_cycles_8080[256];
	static const uint8_t lut_cycles_8085[256];
	uint8_t lut_cycles[256];
	/* flags lookup */
	uint8_t ZS[256];
	uint8_t ZSP[256];

	void set_sod(int state);
	void set_inte(int state);
	void set_status(uint8_t status);
	uint8_t get_rim_value();
	void break_halt_for_interrupt();
	uint8_t ROP();
	uint8_t ARG();
	uint16_t ARG16();
	uint8_t RM(uint32_t a);
	void WM(uint32_t a, uint8_t v);
	void check_for_interrupts();
	void execute_one(int opcode);
	void init_tables();

};


class i8080_cpu_device : public i8085a_cpu_device
{
public:
	// construction/destruction
	i8080_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	virtual uint32_t execute_input_lines() const override { return 1; }
	virtual uint64_t execute_clocks_to_cycles(uint64_t clocks) const override { return clocks; }
	virtual uint64_t execute_cycles_to_clocks(uint64_t cycles) const override { return cycles; }
};


class i8080a_cpu_device : public i8085a_cpu_device
{
public:
	// construction/destruction
	i8080a_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	virtual uint32_t execute_input_lines() const override { return 1; }
	virtual uint64_t execute_clocks_to_cycles(uint64_t clocks) const override { return clocks; }
	virtual uint64_t execute_cycles_to_clocks(uint64_t cycles) const override { return cycles; }
};


extern const device_type I8080;
extern const device_type I8080A;
extern const device_type I8085A;

#endif
