/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Linux Socket Filter Data Structures
 */
#ifndef __LINUX_FILTER_H__
#define __LINUX_FILTER_H__

#include <stdarg.h>

#include <linux/atomic.h>
#include <linux/refcount.h>
#include <linux/compat.h>
#include <linux/skbuff.h>
#include <linux/linkage.h>
#include <linux/printk.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/capability.h>
#include <linux/cryptohash.h>
#include <linux/set_memory.h>
#include <linux/kallsyms.h>
#include <linux/if_vlan.h>
#include <linux/vmalloc.h>

#include <net/sch_generic.h>

#include <uapi/linux/filter.h>
#include <uapi/linux/bpf.h>

struct sk_buff;
struct sock;
struct seccomp_data;
struct bpf_prog_aux;
struct xdp_rxq_info;
struct xdp_buff;
struct sock_reuseport;
struct ctl_table;
struct ctl_table_header;

/* ArgX, context and stack frame pointer register positions. Note,
 * Arg1, Arg2, Arg3, etc are used as argument mappings of function
 * calls in BPF_CALL instruction.
 */
#define BPF_REG_ARG1	BPF_REG_1
#define BPF_REG_ARG2	BPF_REG_2
#define BPF_REG_ARG3	BPF_REG_3
#define BPF_REG_ARG4	BPF_REG_4
#define BPF_REG_ARG5	BPF_REG_5
#define BPF_REG_CTX	BPF_REG_6
#define BPF_REG_FP	BPF_REG_10

/* Additional register mappings for converted user programs. */
#define BPF_REG_A	BPF_REG_0
#define BPF_REG_X	BPF_REG_7
#define BPF_REG_TMP	BPF_REG_2	/* scratch reg */
#define BPF_REG_D	BPF_REG_8	/* data, callee-saved */
#define BPF_REG_H	BPF_REG_9	/* hlen, callee-saved */

/* Kernel hidden auxiliary/helper register. */
#define BPF_REG_AX		MAX_BPF_REG
#define MAX_BPF_EXT_REG		(MAX_BPF_REG + 1)
#define MAX_BPF_JIT_REG		MAX_BPF_EXT_REG

/* unused opcode to mark special call to bpf_tail_call() helper */
#define BPF_TAIL_CALL	0xf0

/* unused opcode to mark call to interpreter with arguments */
#define BPF_CALL_ARGS	0xe0

/* As per nm, we expose JITed images as text (code) section for
 * kallsyms. That way, tools like perf can find it to match
 * addresses.
 */
#define BPF_SYM_ELF_TYPE	't'

/* BPF program can access up to 512 bytes of stack space. */
#define MAX_BPF_STACK	512

/* Helper macros for filter block array initializers. */

/* ALU ops on registers, bpf_add|sub|...: dst_reg += src_reg */

#define BPF_ALU64_REG(OP, DST, SRC)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_OP(OP) | BPF_X,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

#define BPF_ALU32_REG(OP, DST, SRC)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_OP(OP) | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

/* ALU ops on immediates, bpf_add|sub|...: dst_reg += imm32 */

#define BPF_ALU64_IMM(OP, DST, IMM)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_OP(OP) | BPF_K,	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

#define BPF_ALU32_IMM(OP, DST, IMM)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_OP(OP) | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

/* Endianess conversion, cpu_to_{l,b}e(), {l,b}e_to_cpu() */

#define BPF_ENDIAN(TYPE, DST, LEN)				\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_END | BPF_SRC(TYPE),	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = LEN })

/* Short form of mov, dst_reg = src_reg */

#define BPF_MOV64_REG(DST, SRC)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_MOV | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

#define BPF_MOV32_REG(DST, SRC)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_MOV | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = 0 })

/* Short form of mov, dst_reg = imm32 */

#define BPF_MOV64_IMM(DST, IMM)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_MOV | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

#define BPF_MOV32_IMM(DST, IMM)					\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_MOV | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

/* BPF_LD_IMM64 macro encodes single 'load 64-bit immediate' insn */
#define BPF_LD_IMM64(DST, IMM)					\
	BPF_LD_IMM64_RAW(DST, 0, IMM)

#define BPF_LD_IMM64_RAW(DST, SRC, IMM)				\
	((struct bpf_insn) {					\
		.code  = BPF_LD | BPF_DW | BPF_IMM,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = (__u32) (IMM) }),			\
	((struct bpf_insn) {					\
		.code  = 0, /* zero is reserved opcode */	\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = ((__u64) (IMM)) >> 32 })

/* pseudo BPF_LD_IMM64 insn used to refer to process-local map_fd */
#define BPF_LD_MAP_FD(DST, MAP_FD)				\
	BPF_LD_IMM64_RAW(DST, BPF_PSEUDO_MAP_FD, MAP_FD)

/* Short form of mov based on type, BPF_X: dst_reg = src_reg, BPF_K: dst_reg = imm32 */

#define BPF_MOV64_RAW(TYPE, DST, SRC, IMM)			\
	((struct bpf_insn) {					\
		.code  = BPF_ALU64 | BPF_MOV | BPF_SRC(TYPE),	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = IMM })

#define BPF_MOV32_RAW(TYPE, DST, SRC, IMM)			\
	((struct bpf_insn) {					\
		.code  = BPF_ALU | BPF_MOV | BPF_SRC(TYPE),	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = IMM })

/* Direct packet access, R0 = *(uint *) (skb->data + imm32) */

#define BPF_LD_ABS(SIZE, IMM)					\
	((struct bpf_insn) {					\
		.code  = BPF_LD | BPF_SIZE(SIZE) | BPF_ABS,	\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = IMM })

/* Indirect packet access, R0 = *(uint *) (skb->data + src_reg + imm32) */

#define BPF_LD_IND(SIZE, SRC, IMM)				\
	((struct bpf_insn) {					\
		.code  = BPF_LD | BPF_SIZE(SIZE) | BPF_IND,	\
		.dst_reg = 0,					\
		.src_reg = SRC,					\
		.off   = 0,					\
		.imm   = IMM })

/* Memory load, dst_reg = *(uint *) (src_reg + off16) */

#define BPF_LDX_MEM(SIZE, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_LDX | BPF_SIZE(SIZE) | BPF_MEM,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Memory store, *(uint *) (dst_reg + off16) = src_reg */

#define BPF_STX_MEM(SIZE, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_STX | BPF_SIZE(SIZE) | BPF_MEM,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Atomic memory add, *(uint *)(dst_reg + off16) += src_reg */

#define BPF_STX_XADD(SIZE, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_STX | BPF_SIZE(SIZE) | BPF_XADD,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Memory store, *(uint *) (dst_reg + off16) = imm32 */

#define BPF_ST_MEM(SIZE, DST, OFF, IMM)				\
	((struct bpf_insn) {					\
		.code  = BPF_ST | BPF_SIZE(SIZE) | BPF_MEM,	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = IMM })

/* Conditional jumps against registers, if (dst_reg 'op' src_reg) goto pc + off16 */

#define BPF_JMP_REG(OP, DST, SRC, OFF)				\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_OP(OP) | BPF_X,		\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Conditional jumps against immediates, if (dst_reg 'op' imm32) goto pc + off16 */

#define BPF_JMP_IMM(OP, DST, IMM, OFF)				\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_OP(OP) | BPF_K,		\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = IMM })

/* Like BPF_JMP_REG, but with 32-bit wide operands for comparison. */

#define BPF_JMP32_REG(OP, DST, SRC, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_JMP32 | BPF_OP(OP) | BPF_X,	\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Like BPF_JMP_IMM, but with 32-bit wide operands for comparison. */

#define BPF_JMP32_IMM(OP, DST, IMM, OFF)			\
	((struct bpf_insn) {					\
		.code  = BPF_JMP32 | BPF_OP(OP) | BPF_K,	\
		.dst_reg = DST,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = IMM })

/* Unconditional jumps, goto pc + off16 */

#define BPF_JMP_A(OFF)						\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_JA,			\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = OFF,					\
		.imm   = 0 })

/* Relative call */

#define BPF_CALL_REL(TGT)					\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_CALL,			\
		.dst_reg = 0,					\
		.src_reg = BPF_PSEUDO_CALL,			\
		.off   = 0,					\
		.imm   = TGT })

/* Function call */

#define BPF_CAST_CALL(x)					\
		((u64 (*)(u64, u64, u64, u64, u64))(x))

#define BPF_EMIT_CALL(FUNC)					\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_CALL,			\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = ((FUNC) - __bpf_call_base) })

/* Raw code statement block */

#define BPF_RAW_INSN(CODE, DST, SRC, OFF, IMM)			\
	((struct bpf_insn) {					\
		.code  = CODE,					\
		.dst_reg = DST,					\
		.src_reg = SRC,					\
		.off   = OFF,					\
		.imm   = IMM })

/* Program exit */

#define BPF_EXIT_INSN()						\
	((struct bpf_insn) {					\
		.code  = BPF_JMP | BPF_EXIT,			\
		.dst_reg = 0,					\
		.src_reg = 0,					\
		.off   = 0,					\
		.imm   = 0 })

/* Internal classic blocks for direct assignment */

#define __BPF_STMT(CODE, K)					\
	((struct sock_filter) BPF_STMT(CODE, K))

#define __BPF_JUMP(CODE, K, JT, JF)				\
	((struct sock_filter) BPF_JUMP(CODE, K, JT, JF))

#define bytes_to_bpf_size(bytes)				\
({								\
	int bpf_size = -EINVAL;					\
								\
	if (bytes == sizeof(u8))				\
		bpf_size = BPF_B;				\
	else if (bytes == sizeof(u16))				\
		bpf_size = BPF_H;				\
	else if (bytes == sizeof(u32))				\
		bpf_size = BPF_W;				\
	else if (bytes == sizeof(u64))				\
		bpf_size = BPF_DW;				\
								\
	bpf_size;						\
})

#define bpf_size_to_bytes(bpf_size)				\
({								\
	int bytes = -EINVAL;					\
								\
	if (bpf_size == BPF_B)					\
		bytes = sizeof(u8);				\
	else if (bpf_size == BPF_H)				\
		bytes = sizeof(u16);				\
	else if (bpf_size == BPF_W)				\
		bytes = sizeof(u32);				\
	else if (bpf_size == BPF_DW)				\
		bytes = sizeof(u64);				\
								\
	bytes;							\
})

#define BPF_SIZEOF(type)					\
	({							\
		const int __size = bytes_to_bpf_size(sizeof(type)); \
		BUILD_BUG_ON(__size < 0);			\
		__size;						\
	})

#define BPF_FIELD_SIZEOF(type, field)				\
	({							\
		const int __size = bytes_to_bpf_size(FIELD_SIZEOF(type, field)); \
		BUILD_BUG_ON(__size < 0);			\
		__size;						\
	})

#define BPF_LDST_BYTES(insn)					\
	({							\
		const int __size = bpf_size_to_bytes(BPF_SIZE((insn)->code)); \
		WARN_ON(__size < 0);				\
		__size;						\
	})

#define __BPF_MAP_0(m, v, ...) v
#define __BPF_MAP_1(m, v, t, a, ...) m(t, a)
#define __BPF_MAP_2(m, v, t, a, ...) m(t, a), __BPF_MAP_1(m, v, __VA_ARGS__)
#define __BPF_MAP_3(m, v, t, a, ...) m(t, a), __BPF_MAP_2(m, v, __VA_ARGS__)
#define __BPF_MAP_4(m, v, t, a, ...) m(t, a), __BPF_MAP_3(m, v, __VA_ARGS__)
#define __BPF_MAP_5(m, v, t, a, ...) m(t, a), __BPF_MAP_4(m, v, __VA_ARGS__)

#define __BPF_REG_0(...) __BPF_PAD(5)
#define __BPF_REG_1(...) __BPF_MAP(1, __VA_ARGS__), __BPF_PAD(4)
#define __BPF_REG_2(...) __BPF_MAP(2, __VA_ARGS__), __BPF_PAD(3)
#define __BPF_REG_3(...) __BPF_MAP(3, __VA_ARGS__), __BPF_PAD(2)
#define __BPF_REG_4(...) __BPF_MAP(4, __VA_ARGS__), __BPF_PAD(1)
#define __BPF_REG_5(...) __BPF_MAP(5, __VA_ARGS__)

#define __BPF_MAP(n, ...) __BPF_MAP_##n(__VA_ARGS__)
#define __BPF_REG(n, ...) __BPF_REG_##n(__VA_ARGS__)

#define __BPF_CAST(t, a)						       \
	(__force t)							       \
	(__force							       \
	 typeof(__builtin_choose_expr(sizeof(t) == sizeof(unsigned long),      \
				      (unsigned long)0, (t)0))) a
#define __BPF_V void
#define __BPF_N

#define __BPF_DECL_ARGS(t, a) t   a
#define __BPF_DECL_REGS(t, a) u64 a

#define __BPF_PAD(n)							       \
	__BPF_MAP(n, __BPF_DECL_ARGS, __BPF_N, u64, __ur_1, u64, __ur_2,       \
		  u64, __ur_3, u64, __ur_4, u64, __ur_5)

#define BPF_CALL_x(x, name, ...)					       \
	static __always_inline						       \
	u64 ____##name(__BPF_MAP(x, __BPF_DECL_ARGS, __BPF_V, __VA_ARGS__));   \
	u64 name(__BPF_REG(x, __BPF_DECL_REGS, __BPF_N, __VA_ARGS__));	       \
	u64 name(__BPF_REG(x, __BPF_DECL_REGS, __BPF_N, __VA_ARGS__))	       \
	{								       \
		return ____##name(__BPF_MAP(x,__BPF_CAST,__BPF_N,__VA_ARGS__));\
	}								       \
	static __always_inline						       \
	u64 ____##name(__BPF_MAP(x, __BPF_DECL_ARGS, __BPF_V, __VA_ARGS__))

#define BPF_CALL_0(name, ...)	BPF_CALL_x(0, name, __VA_ARGS__)
#define BPF_CALL_1(name, ...)	BPF_CALL_x(1, name, __VA_ARGS__)
#define BPF_CALL_2(name, ...)	BPF_CALL_x(2, name, __VA_ARGS__)
#define BPF_CALL_3(name, ...)	BPF_CALL_x(3, name, __VA_ARGS__)
#define BPF_CALL_4(name, ...)	BPF_CALL_x(4, name, __VA_ARGS__)
#define BPF_CALL_5(name, ...)	BPF_CALL_x(5, name, __VA_ARGS__)

#define bpf_ctx_range(TYPE, MEMBER)						\
	offsetof(TYPE, MEMBER) ... offsetofend(TYPE, MEMBER) - 1
#define bpf_ctx_range_till(TYPE, MEMBER1, MEMBER2)				\
	offsetof(TYPE, MEMBER1) ... offsetofend(TYPE, MEMBER2) - 1
#if BITS_PER_LONG == 64
# define bpf_ctx_range_ptr(TYPE, MEMBER)					\
	offsetof(TYPE, MEMBER) ... offsetofend(TYPE, MEMBER) - 1
#else
# define bpf_ctx_range_ptr(TYPE, MEMBER)					\
	offsetof(TYPE, MEMBER) ... offsetof(TYPE, MEMBER) + 8 - 1
#endif /* BITS_PER_LONG == 64 */

#define bpf_target_off(TYPE, MEMBER, SIZE, PTR_SIZE)				\
	({									\
		BUILD_BUG_ON(FIELD_SIZEOF(TYPE, MEMBER) != (SIZE));		\
		*(PTR_SIZE) = (SIZE);						\
		offsetof(TYPE, MEMBER);						\
	})

#ifdef CONFIG_COMPAT
/* A struct sock_filter is architecture independent. */
struct compat_sock_fprog {
	u16		len;
	compat_uptr_t	filter;	/* struct sock_filter * */
};
#endif

struct sock_fprog_kern {
	u16			len;
	struct sock_filter	*filter;
};

struct bpf_binary_header {
	u32 pages;
	/* Some arches need word alignment for their instructions */
	u8 image[] __aligned(4);
};

struct bpf_prog {
	u16			pages;		/* Number of allocated pages */
	u16			jited:1,	/* Is our filter JIT'ed? */
				jit_requested:1,/* archs need to JIT the prog */
				gpl_compatible:1, /* Is filter GPL compatible? */
				cb_access:1,	/* Is control block accessed? */
				dst_needed:1,	/* Do we need dst entry? */
				blinded:1,	/* Was blinded */
				is_func:1,	/* program is a bpf function */
				kprobe_override:1, /* Do we override a kprobe? */
				has_callchain_buf:1; /* callchain buffer allocated? */
	enum bpf_prog_type	type;		/* Type of BPF program */
	enum bpf_attach_type	expected_attach_type; /* For some prog types */
	u32			len;		/* Number of filter blocks */
	u32			jited_len;	/* Size of jited insns in bytes */
	u8			tag[BPF_TAG_SIZE];
	struct bpf_prog_aux	*aux;		/* Auxiliary fields */
	struct sock_fprog_kern	*orig_prog;	/* Original BPF program */
	unsigned int		(*bpf_func)(const void *ctx,
					    const struct bpf_insn *insn);
	/* Instructions for interpreter */
	union {
		struct sock_filter	insns[0];
		struct bpf_insn		insnsi[0];
	};
};

struct sk_filter {
	refcount_t	refcnt;
	struct rcu_head	rcu;
	struct bpf_prog	*prog;
};

DECLARE_STATIC_KEY_FALSE(bpf_stats_enabled_key);

#define BPF_PROG_RUN(prog, ctx)	({				\
	u32 ret;						\
	cant_sleep();						\
	if (static_branch_unlikely(&bpf_stats_enabled_key)) {	\
		struct bpf_prog_stats *stats;			\
		u64 start = sched_clock();			\
		ret = (*(prog)->bpf_func)(ctx, (prog)->insnsi);	\
		stats = this_cpu_ptr(prog->aux->stats);		\
		u64_stats_update_begin(&stats->syncp);		\
		stats->cnt++;					\
		stats->nsecs += sched_clock() - start;		\
		u64_stats_update_end(&stats->syncp);		\
	} else {						\
		ret = (*(prog)->bpf_func)(ctx, (prog)->insnsi);	\
	}							\
	ret; })

#define BPF_SKB_CB_LEN QDISC_CB_PRIV_LEN

struct bpf_skb_data_end {
	struct qdisc_skb_cb qdisc_cb;
	void *data_meta;
	void *data_end;
};

struct bpf_redirect_info {
	u32 ifindex;
	u32 flags;
	struct bpf_map *map;
	struct bpf_map *map_to_flush;
	u32 kern_flags;
};

DECLARE_PER_CPU(struct bpf_redirect_info, bpf_redirect_info);

/* flags for bpf_redirect_info kern_flags */
#define BPF_RI_F_RF_NO_DIRECT	BIT(0)	/* no napi_direct on return_frame */

/* Compute the linear packet data range [data, data_end) which
 * will be accessed by various program types (cls_bpf, act_bpf,
 * lwt, ...). Subsystems allowing direct data access must (!)
 * ensure that cb[] area can be written to when BPF program is
 * invoked (otherwise cb[] save/restore is necessary).
 */
static inline void bpf_compute_data_pointers(struct sk_buff *skb)
{
	struct bpf_skb_data_end *cb = (struct bpf_skb_data_end *)skb->cb;

	BUILD_BUG_ON(sizeof(*cb) > FIELD_SIZEOF(struct sk_buff, cb));
	cb->data_meta = skb->data - skb_metadata_len(skb);
	cb->data_end  = skb->data + skb_headlen(skb);
}

/* Similar to bpf_compute_data_pointers(), except that save orginal
 * data in cb->data and cb->meta_data for restore.
 */
static inline void bpf_compute_and_save_data_end(
	struct sk_buff *skb, void **saved_data_end)
{
	struct bpf_skb_data_end *cb = (struct bpf_skb_data_end *)skb->cb;

	*saved_data_end = cb->data_end;
	cb->data_end  = skb->data + skb_headlen(skb);
}

/* Restore data saved by bpf_compute_data_pointers(). */
static inline void bpf_restore_data_end(
	struct sk_buff *skb, void *saved_data_end)
{
	struct bpf_skb_data_end *cb = (struct bpf_skb_data_end *)skb->cb;

	cb->data_end = saved_data_end;
}

static inline u8 *bpf_skb_cb(struct sk_buff *skb)
{
	/* eBPF programs may read/write skb->cb[] area to transfer meta
	 * data between tail calls. Since this also needs to work with
	 * tc, that scratch memory is mapped to qdisc_skb_cb's data area.
	 *
	 * In some socket filter cases, the cb unfortunately needs to be
	 * saved/restored so that protocol specific skb->cb[] data won't
	 * be lost. In any case, due to unpriviledged eBPF programs
	 * attached to sockets, we need to clear the bpf_skb_cb() area
	 * to not leak previous contents to user space.
	 */
	BUILD_BUG_ON(FIELD_SIZEOF(struct __sk_buff, cb) != BPF_SKB_CB_LEN);
	BUILD_BUG_ON(FIELD_SIZEOF(struct __sk_buff, cb) !=
		     FIELD_SIZEOF(struct qdisc_skb_cb, data));

	return qdisc_skb_cb(skb)->data;
}

static inline u32 __bpf_prog_run_save_cb(const struct bpf_prog *prog,
					 struct sk_buff *skb)
{
	u8 *cb_data = bpf_skb_cb(skb);
	u8 cb_saved[BPF_SKB_CB_LEN];
	u32 res;

	if (unlikely(prog->cb_access)) {
		memcpy(cb_saved, cb_data, sizeof(cb_saved));
		memset(cb_data, 0, sizeof(cb_saved));
	}

	res = BPF_PROG_RUN(prog, skb);

	if (unlikely(prog->cb_access))
		memcpy(cb_data, cb_saved, sizeof(cb_saved));

	return res;
}

static inline u32 bpf_prog_run_save_cb(const struct bpf_prog *prog,
				       struct sk_buff *skb)
{
	u32 res;

	preempt_disable();
	res = __bpf_prog_run_save_cb(prog, skb);
	preempt_enable();
	return res;
}

static inline u32 bpf_prog_run_clear_cb(const struct bpf_prog *prog,
					struct sk_buff *skb)
{
	u8 *cb_data = bpf_skb_cb(skb);
	u32 res;

	if (unlikely(prog->cb_access))
		memset(cb_data, 0, BPF_SKB_CB_LEN);

	preempt_disable();
	res = BPF_PROG_RUN(prog, skb);
	preempt_enable();
	return res;
}

static __always_inline u32 bpf_prog_run_xdp(const struct bpf_prog *prog,
					    struct xdp_buff *xdp)
{
	/* Caller needs to hold rcu_read_lock() (!), otherwise program
	 * can be released while still running, or map elements could be
	 * freed early while still having concurrent users. XDP fastpath
	 * already takes rcu_read_lock() when fetching the program, so
	 * it's not necessary here anymore.
	 */
	return BPF_PROG_RUN(prog, xdp);
}

static inline u32 bpf_prog_insn_size(const struct bpf_prog *prog)
{
	return prog->len * sizeof(struct bpf_insn);
}

static inline u32 bpf_prog_tag_scratch_size(const struct bpf_prog *prog)
{
	return round_up(bpf_prog_insn_size(prog) +
			sizeof(__be64) + 1, SHA_MESSAGE_BYTES);
}

static inline unsigned int bpf_prog_size(unsigned int proglen)
{
	return max(sizeof(struct bpf_prog),
		   offsetof(struct bpf_prog, insns[proglen]));
}

static inline bool bpf_prog_was_classic(const struct bpf_prog *prog)
{
	/* When classic BPF programs have been loaded and the arch
	 * does not have a classic BPF JIT (anymore), they have been
	 * converted via bpf_migrate_filter() to eBPF and thus always
	 * have an unspec program type.
	 */
	return prog->type == BPF_PROG_TYPE_UNSPEC;
}

static inline u32 bpf_ctx_off_adjust_machine(u32 size)
{
	const u32 size_machine = sizeof(unsigned long);

	if (size > size_machine && size % size_machine == 0)
		size = size_machine;

	return size;
}

static inline bool
bpf_ctx_narrow_access_ok(u32 off, u32 size, u32 size_default)
{
	return size <= size_default && (size & (size - 1)) == 0;
}

#define bpf_classic_proglen(fprog) (fprog->len * sizeof(fprog->filter[0]))

static inline void bpf_prog_lock_ro(struct bpf_prog *fp)
{
	set_vm_flush_reset_perms(fp);
	set_memory_ro((unsigned long)fp, fp->pages);
}

static inline void bpf_jit_binary_lock_ro(struct bpf_binary_header *hdr)
{
	set_vm_flush_reset_perms(hdr);
	set_memory_ro((unsigned long)hdr, hdr->pages);
	set_memory_x((unsigned long)hdr, hdr->pages);
}

static inline struct bpf_binary_header *
bpf_jit_binary_hdr(const struct bpf_prog *fp)
{
	unsigned long real_start = (unsigned long)fp->bpf_func;
	unsigned long addr = real_start & PAGE_MASK;

	return (void *)addr;
}

int sk_filter_trim_cap(struct sock *sk, struct sk_buff *skb, unsigned int cap);
static inline int sk_filter(struct sock *sk, struct sk_buff *skb)
{
	return sk_filter_trim_cap(sk, skb, 1);
}

struct bpf_prog *bpf_prog_select_runtime(struct bpf_prog *fp, int *err);
void bpf_prog_free(struct bpf_prog *fp);

bool bpf_opcode_in_insntable(u8 code);

void bpf_prog_free_linfo(struct bpf_prog *prog);
void bpf_prog_fill_jited_linfo(struct bpf_prog *prog,
			       const u32 *insn_to_jit_off);
int bpf_prog_alloc_jited_linfo(struct bpf_prog *prog);
void bpf_prog_free_jited_linfo(struct bpf_prog *prog);
void bpf_prog_free_unused_jited_linfo(struct bpf_prog *prog);

struct bpf_prog *bpf_prog_alloc(unsigned int size, gfp_t gfp_extra_flags);
struct bpf_prog *bpf_prog_alloc_no_stats(unsigned int size, gfp_t gfp_extra_flags);
struct bpf_prog *bpf_prog_realloc(struct bpf_prog *fp_old, unsigned int size,
				  gfp_t gfp_extra_flags);
void __bpf_prog_free(struct bpf_prog *fp);

static inline void bpf_prog_unlock_free(struct bpf_prog *fp)
{
	__bpf_prog_free(fp);
}

typedef int (*bpf_aux_classic_check_t)(struct sock_filter *filter,
				       unsigned int flen);

int bpf_prog_create(struct bpf_prog **pfp, struct sock_fprog_kern *fprog);
int bpf_prog_create_from_user(struct bpf_prog **pfp, struct sock_fprog *fprog,
			      bpf_aux_classic_check_t trans, bool save_orig);
void bpf_prog_destroy(struct bpf_prog *fp);

int sk_attach_filter(struct sock_fprog *fprog, struct sock *sk);
int sk_attach_bpf(u32 ufd, struct sock *sk);
int sk_reuseport_attach_filter(struct sock_fprog *fprog, struct sock *sk);
int sk_reuseport_attach_bpf(u32 ufd, struct sock *sk);
void sk_reuseport_prog_free(struct bpf_prog *prog);
int sk_detach_filter(struct sock *sk);
int sk_get_filter(struct sock *sk, struct sock_filter __user *filter,
		  unsigned int len);

bool sk_filter_charge(struct sock *sk, struct sk_filter *fp);
void sk_filter_uncharge(struct sock *sk, struct sk_filter *fp);

u64 __bpf_call_base(u64 r1, u64 r2, u64 r3, u64 r4, u64 r5);
#define __bpf_call_base_args \
	((u64 (*)(u64, u64, u64, u64, u64, const struct bpf_insn *)) \
	 __bpf_call_base)

struct bpf_prog *bpf_int_jit_compile(struct bpf_prog *prog);
void bpf_jit_compile(struct bpf_prog *prog);
bool bpf_helper_changes_pkt_data(void *func);

static inline bool bpf_dump_raw_ok(void)
{
	/* Reconstruction of call-sites is dependent on kallsyms,
	 * thus make dump the same restriction.
	 */
	return kallsyms_show_value() == 1;
}

struct bpf_prog *bpf_patch_insn_single(struct bpf_prog *prog, u32 off,
				       const struct bpf_insn *patch, u32 len);
int bpf_remove_insns(struct bpf_prog *prog, u32 off, u32 cnt);

void bpf_clear_redirect_map(struct bpf_map *map);

static inline bool xdp_return_frame_no_direct(void)
{
	struct bpf_redirect_info *ri = this_cpu_ptr(&bpf_redirect_info);

	return ri->kern_flags & BPF_RI_F_RF_NO_DIRECT;
}

static inline void xdp_set_return_frame_no_direct(void)
{
	struct bpf_redirect_info *ri = this_cpu_ptr(&bpf_redirect_info);

	ri->kern_flags |= BPF_RI_F_RF_NO_DIRECT;
}

static inline void xdp_clear_return_frame_no_direct(void)
{
	struct bpf_redirect_info *ri = this_cpu_ptr(&bpf_redirect_info);

	ri->kern_flags &= ~BPF_RI_F_RF_NO_DIRECT;
}

static inline int xdp_ok_fwd_dev(const struct net_device *fwd,
				 unsigned int pktlen)
{
	unsigned int len;

	if (unlikely(!(fwd->flags & IFF_UP)))
		return -ENETDOWN;

	len = fwd->mtu + fwd->hard_header_len + VLAN_HLEN;
	if (pktlen > len)
		return -EMSGSIZE;

	return 0;
}

/* The pair of xdp_do_redirect and xdp_do_flush_map MUST be called in the
 * same cpu context. Further for best results no more than a single map
 * for the do_redirect/do_flush pair should be used. This limitation is
 * because we only track one map and force a flush when the map changes.
 * This does not appear to be a real limitation for existing software.
 */
int xdp_do_generic_redirect(struct net_device *dev, struct sk_buff *skb,
			    struct xdp_buff *xdp, struct bpf_prog *prog);
int xdp_do_redirect(struct net_device *dev,
		    struct xdp_buff *xdp,
		    struct bpf_prog *prog);
void xdp_do_flush_map(void);

void bpf_warn_invalid_xdp_action(u32 act);

#ifdef CONFIG_INET
struct sock *bpf_run_sk_reuseport(struct sock_reuseport *reuse, struct sock *sk,
				  struct bpf_prog *prog, struct sk_buff *skb,
				  u32 hash);
#else
static inline struct sock *
bpf_run_sk_reuseport(struct sock_reuseport *reuse, struct sock *sk,
		     struct bpf_prog *prog, struct sk_buff *skb,
		     u32 hash)
{
	return NULL;
}
#endif

#ifdef CONFIG_BPF_JIT
extern int bpf_jit_enable;
extern int bpf_jit_harden;
extern int bpf_jit_kallsyms;
extern long bpf_jit_limit;

typedef void (*bpf_jit_fill_hole_t)(void *area, unsigned int size);

struct bpf_binary_header *
bpf_jit_binary_alloc(unsigned int proglen, u8 **image_ptr,
		     unsigned int alignment,
		     bpf_jit_fill_hole_t bpf_fill_ill_insns);
void bpf_jit_binary_free(struct bpf_binary_header *hdr);
u64 bpf_jit_alloc_exec_limit(void);
void *bpf_jit_alloc_exec(unsigned long size);
void bpf_jit_free_exec(void *addr);
void bpf_jit_free(struct bpf_prog *fp);

int bpf_jit_get_func_addr(const struct bpf_prog *prog,
			  const struct bpf_insn *insn, bool extra_pass,
			  u64 *func_addr, bool *func_addr_fixed);

struct bpf_prog *bpf_jit_blind_constants(struct bpf_prog *fp);
void bpf_jit_prog_release_other(struct bpf_prog *fp, struct bpf_prog *fp_other);

static inline void bpf_jit_dump(unsigned int flen, unsigned int proglen,
				u32 pass, void *image)
{
	pr_err("flen=%u proglen=%u pass=%u image=%pK from=%s pid=%d\n", flen,
	       proglen, pass, image, current->comm, task_pid_nr(current));

	if (image)
		print_hex_dump(KERN_ERR, "JIT code: ", DUMP_PREFIX_OFFSET,
			       16, 1, image, proglen, false);
}

static inline bool bpf_jit_is_ebpf(void)
{
# ifdef CONFIG_HAVE_EBPF_JIT
	return true;
# else
	return false;
# endif
}

static inline bool ebpf_jit_enabled(void)
{
	return bpf_jit_enable && bpf_jit_is_ebpf();
}

static inline bool bpf_prog_ebpf_jited(const struct bpf_prog *fp)
{
	return fp->jited && bpf_jit_is_ebpf();
}

static inline bool bpf_jit_blinding_enabled(struct bpf_prog *prog)
{
	/* These are the prerequisites, should someone ever have the
	 * idea to call blinding outside of them, we make sure to
	 * bail out.
	 */
	if (!bpf_jit_is_ebpf())
		return false;
	if (!prog->jit_requested)
		return false;
	if (!bpf_jit_harden)
		return false;
	if (bpf_jit_harden == 1 && capable(CAP_SYS_ADMIN))
		return false;

	return true;
}

static inline bool bpf_jit_kallsyms_enabled(void)
{
	/* There are a couple of corner cases where kallsyms should
	 * not be enabled f.e. on hardening.
	 */
	if (bpf_jit_harden)
		return false;
	if (!bpf_jit_kallsyms)
		return false;
	if (bpf_jit_kallsyms == 1)
		return true;

	return false;
}

const char *__bpf_address_lookup(unsigned long addr, unsigned long *size,
				 unsigned long *off, char *sym);
bool is_bpf_text_address(unsigned long addr);
int bpf_get_kallsym(unsigned int symnum, unsigned long *value, char *type,
		    char *sym);

static inline const char *
bpf_address_lookup(unsigned long addr, unsigned long *size,
		   unsigned long *off, char **modname, char *sym)
{
	const char *ret = __bpf_address_lookup(addr, size, off, sym);

	if (ret && modname)
		*modname = NULL;
	return ret;
}

void bpf_prog_kallsyms_add(struct bpf_prog *fp);
void bpf_prog_kallsyms_del(struct bpf_prog *fp);
void bpf_get_prog_name(const struct bpf_prog *prog, char *sym);

#else /* CONFIG_BPF_JIT */

static inline bool ebpf_jit_enabled(void)
{
	return false;
}

static inline bool bpf_prog_ebpf_jited(const struct bpf_prog *fp)
{
	return false;
}

static inline void bpf_jit_free(struct bpf_prog *fp)
{
	bpf_prog_unlock_free(fp);
}

static inline bool bpf_jit_kallsyms_enabled(void)
{
	return false;
}

static inline const char *
__bpf_address_lookup(unsigned long addr, unsigned long *size,
		     unsigned long *off, char *sym)
{
	return NULL;
}

static inline bool is_bpf_text_address(unsigned long addr)
{
	return false;
}

static inline int bpf_get_kallsym(unsigned int symnum, unsigned long *value,
				  char *type, char *sym)
{
	return -ERANGE;
}

static inline const char *
bpf_address_lookup(unsigned long addr, unsigned long *size,
		   unsigned long *off, char **modname, char *sym)
{
	return NULL;
}

static inline void bpf_prog_kallsyms_add(struct bpf_prog *fp)
{
}

static inline void bpf_prog_kallsyms_del(struct bpf_prog *fp)
{
}

static inline void bpf_get_prog_name(const struct bpf_prog *prog, char *sym)
{
	sym[0] = '\0';
}

#endif /* CONFIG_BPF_JIT */

void bpf_prog_kallsyms_del_subprogs(struct bpf_prog *fp);
void bpf_prog_kallsyms_del_all(struct bpf_prog *fp);

#define BPF_ANC		BIT(15)

static inline bool bpf_needs_clear_a(const struct sock_filter *first)
{
	switch (first->code) {
	case BPF_RET | BPF_K:
	case BPF_LD | BPF_W | BPF_LEN:
		return false;

	case BPF_LD | BPF_W | BPF_ABS:
	case BPF_LD | BPF_H | BPF_ABS:
	case BPF_LD | BPF_B | BPF_ABS:
		if (first->k == SKF_AD_OFF + SKF_AD_ALU_XOR_X)
			return true;
		return false;

	default:
		return true;
	}
}

static inline u16 bpf_anc_helper(const struct sock_filter *ftest)
{
	BUG_ON(ftest->code & BPF_ANC);

	switch (ftest->code) {
	case BPF_LD | BPF_W | BPF_ABS:
	case BPF_LD | BPF_H | BPF_ABS:
	case BPF_LD | BPF_B | BPF_ABS:
#define BPF_ANCILLARY(CODE)	case SKF_AD_OFF + SKF_AD_##CODE:	\
				return BPF_ANC | SKF_AD_##CODE
		switch (ftest->k) {
		BPF_ANCILLARY(PROTOCOL);
		BPF_ANCILLARY(PKTTYPE);
		BPF_ANCILLARY(IFINDEX);
		BPF_ANCILLARY(NLATTR);
		BPF_ANCILLARY(NLATTR_NEST);
		BPF_ANCILLARY(MARK);
		BPF_ANCILLARY(QUEUE);
		BPF_ANCILLARY(HATYPE);
		BPF_ANCILLARY(RXHASH);
		BPF_ANCILLARY(CPU);
		BPF_ANCILLARY(ALU_XOR_X);
		BPF_ANCILLARY(VLAN_TAG);
		BPF_ANCILLARY(VLAN_TAG_PRESENT);
		BPF_ANCILLARY(PAY_OFFSET);
		BPF_ANCILLARY(RANDOM);
		BPF_ANCILLARY(VLAN_TPID);
		}
		/* Fallthrough. */
	default:
		return ftest->code;
	}
}

void *bpf_internal_load_pointer_neg_helper(const struct sk_buff *skb,
					   int k, unsigned int size);

static inline void *bpf_load_pointer(const struct sk_buff *skb, int k,
				     unsigned int size, void *buffer)
{
	if (k >= 0)
		return skb_header_pointer(skb, k, size, buffer);

	return bpf_internal_load_pointer_neg_helper(skb, k, size);
}

static inline int bpf_tell_extensions(void)
{
	return SKF_AD_MAX;
}

struct bpf_sock_addr_kern {
	struct sock *sk;
	struct sockaddr *uaddr;
	/* Temporary "register" to make indirect stores to nested structures
	 * defined above. We need three registers to make such a store, but
	 * only two (src and dst) are available at convert_ctx_access time
	 */
	u64 tmp_reg;
	void *t_ctx;	/* Attach type specific context. */
};

struct bpf_sock_ops_kern {
	struct	sock *sk;
	u32	op;
	union {
		u32 args[4];
		u32 reply;
		u32 replylong[4];
	};
	u32	is_fullsock;
	u64	temp;			/* temp and everything after is not
					 * initialized to 0 before calling
					 * the BPF program. New fields that
					 * should be initialized to 0 should
					 * be inserted before temp.
					 * temp is scratch storage used by
					 * sock_ops_convert_ctx_access
					 * as temporary storage of a register.
					 */
};

struct bpf_sysctl_kern {
	struct ctl_table_header *head;
	struct ctl_table *table;
	void *cur_val;
	size_t cur_len;
	void *new_val;
	size_t new_len;
	int new_updated;
	int write;
	loff_t *ppos;
	/* Temporary "register" for indirect stores to ppos. */
	u64 tmp_reg;
};

#endif /* __LINUX_FILTER_H__ */
