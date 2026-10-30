/*
 * desk_rsc_data.h - packed EmuDesk resource tables (generated)
 */
#ifndef DESK_RSC_DATA_H
#define DESK_RSC_DATA_H

/* Requires emutos.h / portab.h for UBYTE, UWORD, WORD, LONG. */

#define DESK_RS_NFSTR   56
#define DESK_RS_NFLAG   19
#define DESK_RS_NIMM    28
#define DESK_RS_NHI     94
#define DESK_RS_POOLSZ  5443
#define DESK_RS_TEBUFSZ 389
#define DESK_RS_BMWORDS 64
#define DESK_RS_NPOOL   218

extern const UBYTE desk_rs_nchild[];
extern const UBYTE desk_rs_otype[];
extern const UBYTE desk_rs_oflagi[];
extern const UBYTE desk_rs_ostate[];
extern const UBYTE desk_rs_xylo[];
extern const UBYTE desk_rs_xyhi[];
extern const UWORD desk_rs_flagdict[];
extern const LONG desk_rs_immdict[];
extern const UBYTE desk_rs_ospec[];
extern const UBYTE desk_rs_tedpack[];
extern const UBYTE desk_rs_pool[];
extern const UWORD desk_rs_pooloff[];
extern const UWORD desk_rs_fstridx[];
extern const UWORD desk_rs_treestart[];
extern const UWORD desk_rs_bmdata[];
extern const WORD desk_rs_bminfo[];

#endif /* DESK_RSC_DATA_H */
