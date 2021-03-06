// ported from SGOS1

#include <sgos.h>
#include <arch.h>
#include <rtl.h>
#include <kd.h>

#define MAX_IDT 256
#define MAX_GDT 256

static SEGMENT_DESC	gdt[MAX_GDT];	//	256 gdt items
static GDT_ADDR		gdt_ptr;
static GATE_DESC	idt[MAX_IDT];	//	256 gdt items
static GDT_ADDR		idt_ptr;

static t_8		realmode_idt_mask;			//中断屏蔽位图

static t_16		idt_code_seg = 0x8;	//default is 0x8


//设置门描述符
void ArSetGate( int vector, uchar desc_type, void* handle )
{
	ArSetIdtEntry( vector, desc_type, handle );
}

//设置Global Descriptor Table项
void ArSetGdtEntry( int vector, uint base, uint limit, uint attribute )
{
	SEGMENT_DESC* desc	=	&gdt[vector];
	desc->limit_low		=	limit & 0x0FFFF;
	desc->base_low		=	base & 0x0FFFF;
	desc->base_mid		= 	(base >> 16) & 0x0FF;	
	desc->attr1		=	attribute & 0xFF;
	desc->limit_high_attr2	=	( (limit >> 16) & 0x0F ) | ((attribute >> 8) & 0xF0);
	desc->base_high		=	(base >> 24) & 0x0FF;
}

//设置Interrupt Descriptor Table项
void ArSetIdtEntry( int vector, uchar desc_type, void* handler )
{
	t_32 base		=	(t_32)handler;
	GATE_DESC* desc		=	&idt[vector];
	desc->offset_low	=	base & 0xFFFF;
	desc->selector		=	idt_code_seg;	//系统代码段
	desc->dcount		=	0;
	desc->attr		=	desc_type ;
	desc->offset_high	=	(base >> 16) & 0xFFFF;
}

//设置Local Descriptor Table项
void ArSetLdtEntry( SEGMENT_DESC *desc, uint base, uint limit, uint attribute )
{
	desc->limit_low		=	limit & 0x0FFFF;	
	desc->base_low		=	base & 0x0FFFF;
	desc->base_mid		=	(base >> 16) & 0x0FF;
	desc->attr1		=	(attribute & 0xFF);
	desc->limit_high_attr2	=	((limit >> 16) & 0x0F) | ((attribute >> 8) & 0xF0);
	desc->base_high		=	(base >> 24) & 0x0FF;
}


//保护模式编程
void ArInitializeGdt()
{
	idt_code_seg = 0x08;
	// setup a new gdt
	/* 但这里有必要再做一次这个工作，便于管理。 */
	RtlZeroMemory( gdt, sizeof(gdt) );
	RtlZeroMemory( idt, sizeof(idt) );
	// 初始化一个代码段和一个数据段，内核态的线程使用
	ArSetGdtEntry( 1, 0x0000, 0xFFFFF, DA_CR | DA_32 | DA_LIMIT_4K );
	ArSetGdtEntry( 2, 0x0000, 0xFFFFF, DA_DRW | DA_LIMIT_4K | DA_32 );
	// 用户态线程使用
	ArSetGdtEntry( 3, 0x0000, 0xFFFFF, DA_CR | DA_32 | DA_LIMIT_4K | DA_DPL3 );
	ArSetGdtEntry( 4, 0x0000, 0xFFFFF, DA_DRW | DA_LIMIT_4K | DA_32 | DA_DPL3 );

	gdt_ptr.limit = sizeof(SEGMENT_DESC)*256;
	gdt_ptr.addr = (t_32)gdt;
	
	// 内嵌汇编载入 gdt 表描述符
	__asm__ __volatile__ ( "lgdt %0" : "=m"( gdt_ptr ) ) ; //载入GDT表

	// 保存实模式中断屏蔽寄存器(IMREG)值
	realmode_idt_mask = ArInByte( 0x21 );
	//
	idt_ptr.limit = sizeof(idt);
	idt_ptr.addr = (t_32)idt;
	// 内嵌汇编载入 idt 表描述符
	__asm__ __volatile__  ( "lidt %0" : "=m"( idt_ptr ) ) ; //
}

