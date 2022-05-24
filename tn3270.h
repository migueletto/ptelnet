// Prototipos

Err Init0TN3270(Int16, Int16) SEC("tn3270");
Err InitTN3270(Int16, Int16) SEC("tn3270");
void CloseTN3270(void) SEC("tn3270");
void EmitTN3270(Char *, Int16, Int16) SEC("tn3270");
Err SendTN3270(UInt16) SEC("tn3270");
Err SendAID(UInt16) SEC("tn3270");
Err SendSpecial(UInt16) SEC("tn3270");
void PositionTN3270(Int16 col, Int16 row) SEC("tn3270");

// Constantes

#define	ESC		27

#define CMD_W		  0x01	// Write
#define CMD_EW		0x05	// Erase/Write
#define CMD_EWA		0x0d	// Erase/Write Alternate
#define CMD_RB		0x02	// Read Buffer
#define CMD_RM		0x06	// Read Modified
#define CMD_RMA		0x0e	// Read Modified All
#define CMD_EAU		0x0f	// Erase All Unprotected
#define CMD_WSF		0x11	// Write Structured Field

#define SNA_CMD_W	  0xf1	// Write
#define SNA_CMD_EW	0xf5	// Erase/Write
#define SNA_CMD_EWA	0x7e	// Erase/Write Alternate
#define SNA_CMD_RB	0xf2	// Read Buffer
#define SNA_CMD_RM	0xf6	// Read Modified
#define SNA_CMD_RMA	0x6e	// Read Modified All
#define SNA_CMD_EAU	0x6f	// Erase All Unprotected
#define SNA_CMD_WSF	0xf3	// Write Structured Field

// Orders

#define ORD_SF		0x1d	// Start Field
#define ORD_SFE		0x29	// Start Field Extended
#define ORD_SBA		0x11	// Start Buffer Address
#define ORD_SA		0x28	// Set Attribute
#define ORD_MF		0x2c	// Modify Field
#define ORD_IC		0x13	// Insert Cursor
#define ORD_PT		0x05	// Program Tab
#define ORD_RA		0x3c	// Repeat to Address
#define ORD_EUA		0x12	// Erase Unprotected to Address
#define ORD_GE		0x08	// Graphic Escape

// Control Orders

#define CORD_NUL	0x00	// Null: blank
#define CORD_SUB	0x3f	// Substitute: solid circle
#define CORD_DUP	0x1c	// Duplicate: overscore asterisk
#define CORD_FM		0x1e	// Field mark: overscore semicolon
#define CORD_FF		0x0c	// Form feed: blank
#define CORD_CR		0x0d	// Carriage return: blank
#define CORD_NL		0x15	// New line: blank
#define CORD_EM		0x19	// End of medium: blank
#define CORD_EO		0xff	// Eight ones: blank

// Buffer address

#define BA_MODE(b)	((b) >> 6)

#define BA_14		0x0	// 14-bit binary address
#define BA_12A		0x1	// 12-bit coded binary address
#define BA_RESERVED	0x2	// reserved
#define BA_12B		0x3	// 12-bit coded binary address

// Field Attributes

#define FA_PROTECTED(b)    ((b) & 0x20)
#define FA_DISPLAY(b)     (((b) & 0x0c) != 0x0c)
#define FA_INTENSIFIED(b) (((b) & 0x0c) == 0x08)
#define FA_DETECTABLE(b)  (((b) & 0x0c) == 0x08 || ((b) & 0x0c) == 0x04)
#define FA_MDT(b)          ((b) & 0x01)
#define FA_SETMDT(b)       ((b) | 0x01)
#define FA_RESETMDT(b)     ((b) & 0xfe)

// Attention Identification

#define AID_NO		0x60	// No AID generated
#define AID_NOPR	0xe8	// No AID generated (printer only)
#define AID_SF		0x88	// Structured field
#define AID_RP		0x61	// Read partition
#define AID_TA		0x7f	// Trigger action
#define AID_TRSR	0xf0	// Test Req and Sys Req
#define AID_PF1		0xf1	// PF1 key
#define AID_PF2		0xf2	// PF2 key
#define AID_PF3		0xf3	// PF3 key
#define AID_PF4		0xf4	// PF4 key
#define AID_PF5		0xf5	// PF5 key
#define AID_PF6		0xf6	// PF6 key
#define AID_PF7		0xf7	// PF7 key
#define AID_PF8		0xf8	// PF8 key
#define AID_PF9		0xf9	// PF9 key
#define AID_PF10	0x7a	// PF10 key
#define AID_PF11	0x7b	// PF11 key
#define AID_PF12	0x7c	// PF12 key
#define AID_PF13	0xc1	// PF13 key
#define AID_PF14	0xc2	// PF14 key
#define AID_PF15	0xc3	// PF15 key
#define AID_PF16	0xc4	// PF16 key
#define AID_PF17	0xc5	// PF17 key
#define AID_PF18	0xc6	// PF18 key
#define AID_PF19	0xc7	// PF19 key
#define AID_PF20	0xc8	// PF20 key
#define AID_PF21	0xc9	// PF21 key
#define AID_PF22	0x4a	// PF22 key
#define AID_PF23	0x4b	// PF23 key
#define AID_PF24	0x4c	// PF24 key
#define AID_PA1		0x6c	// PA1 key
#define AID_PA2		0x6e	// PA2 key (Cncl)
#define AID_PA3		0x6b	// PA3 key
#define AID_CLEAR	0x6d	// Clear key
#define AID_CLEARP	0x6a	// Clear Partition key
#define AID_ENTER	0x7d	// Enter key
#define AID_SPA		0x7e	// Selector pen attention
#define AID_OIR		0xe6	// Magnetic readers: operator ID reader
#define AID_MRN		0xe7	// Magnetic readers: mag. reader number
