/* TODO: spec citations per structure */

#include <drivers/nvme.h>
#include <drivers/verifier.h>

#define DV_STRUCT nvme_regs
dv_size(0x1020);
dv_field(uint32_t, cap_lo, 0x00);
dv_field(uint32_t, cap_hi, 0x04);
dv_field(uint32_t, version, 0x08);
dv_field(uint32_t, intms, 0x0C);
dv_field(uint32_t, intmc, 0x10);
dv_field(struct nvme_cc, cc, 0x14);
dv_field(uint32_t, nssr, 0x18);
dv_field(uint32_t, csts, 0x1C);
dv_field(uint32_t, reserved1, 0x20);
dv_field(uint32_t, aqa, 0x24);
dv_field(uint32_t, asq_lo, 0x28);
dv_field(uint32_t, asq_hi, 0x2C);
dv_field(uint32_t, acq_lo, 0x30);
dv_field(uint32_t, acq_hi, 0x34);
dv_field(uint32_t[1018], reserved4, 0x38);
#undef DV_STRUCT

#define DV_STRUCT nvme_command
dv_size(0x40);
dv_align(1);
dv_field(uint8_t, opc, 0x00);
dv_field(uint8_t, fuse, 0x01);
dv_field(uint16_t, cid, 0x02);
dv_field(uint32_t, nsid, 0x04);
dv_field(uint64_t, rsvd2, 0x08);
dv_field(uint64_t, mptr, 0x10);
dv_field(uint64_t, prp1, 0x18);
dv_field(uint64_t, prp2, 0x20);
dv_field(uint32_t, cdw10, 0x28);
dv_field(uint32_t, cdw11, 0x2C);
dv_field(uint32_t, cdw12, 0x30);
dv_field(uint32_t, cdw13, 0x34);
dv_field(uint32_t, cdw14, 0x38);
dv_field(uint32_t, cdw15, 0x3C);
#undef DV_STRUCT

#define DV_STRUCT nvme_completion
dv_size(0x10);
dv_align(1);
dv_field(uint32_t, result, 0x00);
dv_field(uint32_t, rsvd, 0x04);
dv_field(uint16_t, sq_head, 0x08);
dv_field(uint16_t, sq_id, 0x0A);
dv_field(uint16_t, cid, 0x0C);
dv_field(uint16_t, status, 0x0E);
#undef DV_STRUCT

#define DV_STRUCT nvme_cc
dv_size(0x04);
dv_align(1);
#undef DV_STRUCT

#define DV_STRUCT nvme_lbaf
dv_size(0x04);
dv_align(1);
dv_field(uint16_t, ms, 0x00);
dv_field(uint8_t, lbads, 0x02);
/* bitfields: rp, reserved */
#undef DV_STRUCT

#define DV_STRUCT nvme_identify_namespace
dv_size(0x1000);
dv_align(1);
dv_field(uint64_t, nsze, 0x00);
dv_field(uint64_t, ncap, 0x08);
dv_field(uint64_t, nuse, 0x10);
dv_field(uint8_t, nsfeat, 0x18);
dv_field(uint8_t, nlbaf, 0x19);
dv_field(uint8_t, flbas, 0x1A);
dv_field(uint8_t, mc, 0x1B);
dv_field(uint8_t, dpc, 0x1C);
dv_field(uint8_t, dps, 0x1D);
dv_field(uint8_t, nmic, 0x1E);
dv_field(uint8_t, rescap, 0x1F);
dv_field(uint8_t, fpi, 0x20);
dv_field(uint8_t, dlfeat, 0x21);
dv_field(uint16_t, nawun, 0x22);
dv_field(uint16_t, nawupf, 0x24);
dv_field(uint16_t, nacwu, 0x26);
dv_field(uint16_t, nabsn, 0x28);
dv_field(uint16_t, nabo, 0x2A);
dv_field(uint16_t, nabspf, 0x2C);
dv_field(uint16_t, noiob, 0x2E);
dv_field(uint64_t[2], nvmcap, 0x30);
dv_field(uint16_t, npwg, 0x40);
dv_field(uint16_t, npwa, 0x42);
dv_field(uint16_t, npdg, 0x44);
dv_field(uint16_t, npda, 0x46);
dv_field(uint16_t, nows, 0x48);
dv_field(uint16_t, mssrl, 0x4A);
dv_field(uint32_t, mcl, 0x4C);
dv_field(uint8_t, msrc, 0x50);
dv_field(uint8_t[11], reserved0, 0x51);
dv_field(uint32_t, adagrpid, 0x5C);
dv_field(uint8_t[3], reserved1, 0x60);
dv_field(uint8_t, nsattr, 0x63);
dv_field(uint16_t, nvmsetid, 0x64);
dv_field(uint16_t, endgid, 0x66);
dv_field(uint64_t[2], nguid, 0x68);
dv_field(uint64_t, eui64, 0x78);
dv_field(struct nvme_lbaf[64], lbaf, 0x80);
dv_field(uint8_t[3712], vendor_specific, 0x180);
#undef DV_STRUCT

#define DV_STRUCT nvme_identify_controller
dv_size(0x202);
dv_align(1);
dv_field(uint16_t, vid, 0x00);
dv_field(uint16_t, ssvid, 0x02);
dv_field(char[20], sn, 0x04);
dv_field(char[40], mn, 0x18);
dv_field(char[8], fr, 0x40);
dv_field(uint8_t, rab, 0x48);
dv_field(uint8_t[3], ieee, 0x49);
dv_field(uint8_t, mic, 0x4C);
dv_field(uint8_t, mdts, 0x4D);
dv_field(uint16_t, cntlid, 0x4E);
dv_field(uint32_t, ver, 0x50);
dv_field(uint32_t, rtd3r, 0x54);
dv_field(uint32_t, rtd3e, 0x58);
dv_field(uint32_t, oaes, 0x5C);
dv_field(uint32_t, ctratt, 0x60);
dv_field(uint8_t[156], rsvd96, 0x64);
dv_field(uint16_t, oacs, 0x100);
dv_field(uint8_t, acl, 0x102);
dv_field(uint8_t, aerl, 0x103);
dv_field(uint8_t, frmw, 0x104);
dv_field(uint8_t, lpa, 0x105);
dv_field(uint8_t, elpe, 0x106);
dv_field(uint8_t, npss, 0x107);
dv_field(uint8_t, avscc, 0x108);
dv_field(uint8_t, apsta, 0x109);
dv_field(uint16_t, wctemp, 0x10A);
dv_field(uint16_t, cctemp, 0x10C);
dv_field(uint16_t, mtfa, 0x10E);
dv_field(uint32_t, hmpre, 0x110);
dv_field(uint32_t, hmmin, 0x114);
dv_field(uint64_t[2], tnvmcap, 0x118);
dv_field(uint64_t[2], unvmcap, 0x128);
dv_field(uint32_t, rpmbs, 0x138);
dv_field(uint16_t, edstt, 0x13C);
dv_field(uint8_t, dsto, 0x13E);
dv_field(uint8_t, fwug, 0x13F);
dv_field(uint16_t, kas, 0x140);
dv_field(uint16_t, hctma, 0x142);
dv_field(uint16_t, mntmt, 0x144);
dv_field(uint16_t, mxtmt, 0x146);
dv_field(uint32_t, sanicap, 0x148);
dv_field(uint8_t[180], rsvd228, 0x14C);
dv_field(uint8_t, sqes, 0x200);
dv_field(uint8_t, cqes, 0x201);
#undef DV_STRUCT
