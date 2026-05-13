/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */

#include "scene_setparam_inner.h"
#include <unistd.h>
#include <string.h>
#include "securec.h"
#include "ss_mpi_ae.h"
#include "ss_mpi_venc.h"
#include "ot_common_rc.h"
#include "ss_mpi_vpss.h"
#include "ss_mpi_vi.h"
#include "ot_scenecomm.h"
#include "ot_math.h"
#include "ss_mpi_isp.h"

#ifdef __cplusplus
extern "C" {
#endif


static volatile td_bool g_isp_pause = TD_FALSE;
static volatile td_bool g_run_once = TD_FALSE;
ot_scene_pipe_param g_scene_pipe_param[OT_SCENE_PIPETYPE_NUM];

ot_scene_pipe_param *get_pipe_params(td_void)
{
    return g_scene_pipe_param;
}

td_bool get_isp_pause(td_void)
{
    return g_isp_pause;
}
td_void set_isp_pause(td_bool val)
{
    g_isp_pause = val;
}
td_bool get_run_once(td_void)
{
    return g_run_once;
}
td_void set_run_once(td_bool val)
{
    g_run_once = val;
}

td_u32 scene_get_level_ltoh(td_u64 value, td_u32 count, const td_u64 *thresh, td_u32 thresh_size)
{
    td_u32 level;

    for (level = 0; level < count && level < thresh_size; level++) {
        if (value <= thresh[level]) {
            break;
        }
    }

    if (level == count) {
        level = count - 1;
    }
    return level;
}

td_u32 scene_get_level_ltoh_u32(td_u32 value, td_u32 count, const td_u32 *thresh, td_u32 thresh_size)
{
    td_u32 level;

    for (level = 0; level < count && level < thresh_size; level++) {
        if (value <= thresh[level]) {
            break;
        }
    }

    if (level == count) {
        level = count - 1;
    }
    return level;
}

td_u32 scene_interpulate(td_u64 middle, td_u64 left, td_u64 left_value, td_u64 right, td_u64 right_value)
{
    td_u32 value_tmp, k;

    if (middle <= left) {
        value_tmp = (td_u32)left_value;
        return value_tmp;
    }
    if (middle >= right) {
        value_tmp = (td_u32)right_value;
        return value_tmp;
    }

    k = (td_u32)(right - left);
    value_tmp = (td_u32)(((right - middle) * left_value + (middle - left) * right_value + (k >> 1)) / k);
    return value_tmp;
}

td_s32 scene_interpulate_signed(td_s64 middle, td_s64 left, td_s64 left_value, td_s64 right, td_s64 right_value)
{
    td_s32 value_tmp, k;

    if (middle <= left) {
        value_tmp = (td_s32)left_value;
        return value_tmp;
    }
    if (middle >= right) {
        value_tmp = (td_s32)right_value;
        return value_tmp;
    }

    k = (td_s32)(right - left);

    value_tmp = (td_s32)(((right - middle) * left_value + (middle - left) * right_value - (k / 2)) / k); /* 2 half */

    return value_tmp;
}


td_u32 scene_time_filter(td_u32 param0, td_u32 param1, td_u32 time_cnt, td_u32 index)
{
    td_u64 temp;
    td_u32 value;

    if (param0 > param1) {
        temp = (td_u64)(param0 - param1) << 8;                         /* 8 is param to time_cnt */
        temp = ((temp * (index + 1)) / scene_div_0to1(time_cnt)) >> 8; /* 8 is time_cnt to param */
        value = (param0 > ((td_u32)temp + 1)) ? (param0 - (td_u32)temp - 1) : 0;
    } else if (param0 < param1) {
        temp = (td_u64)(param1 - param0) << 8;                         /* 8 is param to time_cnt */
        temp = ((temp * (index + 1)) / scene_div_0to1(time_cnt)) >> 8; /* 8 is time_cnt to param */
        value = (param0 + (td_u32)temp + 1) > param1 ? param1 : (param0 + (td_u32)temp + 1);
    } else {
        value = param0;
    }
    return value;
}

static void print_nrx1(const ot_scene_3dnr *p_x)
{
    const t_nr_v2_pshrp *pi = &(p_x->iey);
    const t_nr_v2_sfy *ps = p_x->sfy;

    printf("\n -en                  %3u  |              %3u  |              %3u  |               %3u  |", p_x->nry1_en,
        p_x->nry2_en, p_x->nry3_en, p_x->nry4_en);
    printf("\n -nXsf1       %3u:    %3u  |      %3u:    %3u  |      %3u:    %3u  |           %3u:%3u  |", ps[0].sfs1,
        ps[0].sbr1, ps[1].sfs1, ps[1].sbr1, ps[2].sfs1, ps[2].sbr1, pi->coarse_g1, pi->fine_g1); /* 2 index */
    printf("\n -nXsf2       %3u:%3u:%3u  |      %3u:%3u:%3u  |      %3u:%3u:%3u  |           %3u:%3u  |", ps[0].sfs2,
        ps[0].sft2, ps[0].sbr2, ps[1].sfs2, ps[1].sft2, ps[1].sbr2, ps[2].sfs2, ps[2].sft2, ps[2].sbr2, /* 2 index */
        pi->coarse_g2, pi->fine_g2);
    printf("\n -nXsf3                    |      %3u:%3u      |      %3u:%3u      |           %3u:%3u  |", ps[1].strf3,
        ps[1].strb3, ps[2].strf3, ps[2].strb3, pi->coarse_g3, pi->fine_g3); /* 2 index */
    printf("\n -nXsf4       %3u:%3u:%3u  |      %3u:%3u:%3u  |      %3u:%3u:%3u  |           %3u:%3u  |", ps[0].sfs4,
        ps[0].sft4, ps[0].sbr4, ps[1].sfs4, ps[1].sft4, ps[1].sbr4, ps[2].sfs4, ps[2].sft4, ps[2].sbr4, /* 2 index */
        pi->coarse_g4, pi->fine_g4);
    printf("\n -nXsfk4                   |      %3u:%3u      |      %3u:%3u      |                    |", ps[1].strf4,
        ps[1].strb4, ps[2].strf4, ps[2].strb4); /* 2 index */
    printf("\n -nXsf5                    |              %3u  |                   |                    |",
        ps[1].sfs5); /* 1 index */
    printf("\n -nXsf6       %3u:%3u:%3u  |          %3u:%3u  |          %3u:%3u  |                    |", ps[0].sfn6_0,
        ps[0].sfn6_1, ps[0].bld6, ps[1].sfn6_0, ps[1].sfn6_1, ps[2].sfn6_0, ps[2].sfn6_1); /* 2 index */
    printf("\n -nXsf7                    |          %3u:%3u  |          %3u:%3u  |           %3u:%3u  |", ps[1].sfn7_0,
        ps[1].sfn7_1, ps[2].sfn7_0, ps[2].sfn7_1, pi->sfn7_0, pi->sfn7_1); /* 2 index */
    printf("\n -nXsf8                    |          %3u:%3u  |          %3u:%3u  |           %3u:%3u  |", ps[1].sfn8_0,
        ps[1].sfn8_1, ps[2].sfn8_0, ps[2].sfn8_1, pi->sfn8_0, pi->sfn8_1); /* 2 index */
    printf("\n -nXsht                    |                   |                   |  %3u:%3u | %3u:%3u |", pi->o_sht_f,
        pi->u_sht_f, pi->o_sht_b, pi->u_sht_b);
    printf("\n -nXsfn   %3u:%3u:%3u:%3u  |  %3u:%3u:%3u:%3u  |  %3u:%3u:%3u:%3u  |  %3u:%3u:%3u:%3u   |", ps[0].sfn0_0,
        ps[0].sfn1_0, ps[0].sfn2_0, ps[0].sfn3_0, ps[1].sfn0_0, ps[1].sfn1_0, ps[1].sfn2_0, ps[1].sfn3_0,
        ps[2].sfn0_0, ps[2].sfn1_0, ps[2].sfn2_0, ps[2].sfn3_0, pi->sfn0_0, pi->sfn1_0, pi->sfn2_0, /* 2 index */
        pi->sfn3_0);
    printf("\n -nXsth       %3u:%3u:%3u  |      %3u:%3u:%3u  |      %3u:%3u:%3u  |      %3u:%3u:%3u   |", ps[0].sth1_0,
        ps[0].sth2_0, ps[0].sth3_0, ps[1].sth1_0, ps[1].sth2_0, ps[1].sth3_0,
        ps[2].sth1_0, ps[2].sth2_0, ps[2].sth3_0, pi->sth1_0, pi->sth2_0, pi->sth3_0); /* 2 index */
    printf("\n -nX2sfn  %3u:%3u:%3u:%3u  |  %3u:%3u:%3u:%3u  |  %3u:%3u:%3u:%3u  |  %3u:%3u:%3u:%3u   |", ps[0].sfn0_1,
        ps[0].sfn1_1, ps[0].sfn2_1, ps[0].sfn3_1, ps[1].sfn0_1, ps[1].sfn1_1, ps[1].sfn2_1, ps[1].sfn3_1,
        ps[2].sfn0_1, ps[2].sfn1_1, ps[2].sfn2_1, ps[2].sfn3_1, /* 2 index */
        pi->sfn0_1, pi->sfn1_1, pi->sfn2_1, pi->sfn3_1);
    printf("\n -nX2sth      %3u:%3u:%3u  |      %3u:%3u:%3u  |      %3u:%3u:%3u  |      %3u:%3u:%3u   |", ps[0].sth1_1,
        ps[0].sth2_1, ps[0].sth3_1, ps[1].sth1_1, ps[1].sth2_1, ps[1].sth3_1,
        ps[2].sth1_1, ps[2].sth2_1, ps[2].sth3_1, pi->sth1_1, pi->sth2_1, pi->sth3_1); /* 2 index */
}

static void print_nrx2(const ot_scene_3dnr *p_x)
{
    const t_nr_v2_mdy *pm = p_x->mdy;
    const t_nr_v2_nrc1 *pc1 = &(p_x->nrc1);
    const t_nr_v2_nrc0 *pc0 = &(p_x->nrc0);
    const t_nr_v2_mdy0 *pm0 = &(p_x->mdy0);
    const t_nr_v2_tfy *pt = p_x->tfy;

    printf("\n -ref          %3u         |                   |                                        |",
           pt[1].ref_en);
    printf("\n -tfs_mode     %3u         |                   |********************nr_c0***************|",
        pt[0].tfs_mode);
    printf("\n -nXtss     %3u:%3u:%3u    |     %3u:%3u:%3u   |  -nC0mode           %3u                |",
           pt[0].tss0, pt[0].tss1, pt[0].tss2, pt[1].tss0, pt[1].tss1, pt[1].tss2, p_x->nrc0_mode);
    printf("\n -nXtfs     %3u:%3u:%3u    |     %3u:%3u:%3u   |  -tfs               %3u                |",
           pt[0].tfs0, pt[0].tfs1, pt[0].tfs2, pt[1].tfs0, pt[1].tfs1, pt[1].tfs2, pc0->tfs);
    printf("\n -nXtfr0    %3u:%3u:%3u    |     %3u:%3u:%3u   |  -sfc               %3u                |",
           pt[0].tfr0[0], pt[0].tfr0[1], pt[0].tfr0[TFR_2], pt[1].tfr0[TFR_0], pt[1].tfr0[1],
           pt[1].tfr0[TFR_2], pc0->sfc);
    printf("\n            %3u:%3u:%3u    |     %3u:%3u:%3u   |  -tfc               %3u                |",
           pt[0].tfr0[TFR_3], pt[0].tfr0[TFR_4], pt[0].tfr0[TFR_5], pt[1].tfr0[TFR_3], pt[1].tfr0[TFR_4],
           pt[1].tfr0[TFR_5], pc0->tfc);
    printf("\n -nXtfr1    %3u:%3u:%3u    |     %3u:%3u:%3u   |  -trc               %3u                |",
           pt[0].tfr1[0], pt[0].tfr1[1], pt[0].tfr1[TFR_2], pt[1].tfr1[0], pt[1].tfr1[1], pt[1].tfr1[TFR_2], pc0->trc);
    printf("\n            %3u:%3u:%3u    |     %3u:%3u:%3u   |                                        |",
           pt[0].tfr1[TFR_3], pt[0].tfr1[TFR_4], pt[0].tfr1[TFR_5], pt[1].tfr1[TFR_3], pt[1].tfr1[TFR_4],
           pt[1].tfr1[TFR_5]);
    printf("\n -mXmath    %3u:%3u        |     %3u:%3u       |                                        |",
           pm[0].math0, pm[0].math1, pm[1].math0, pm[1].math1);
    printf("\n -mXmate     %3u           |     %3u:%3u       |                                        |",
           pm[0].mate1, pm[1].mate0, pm[1].mate1);
    printf("\n -mXmabw     %3u           |     %3u:%3u       |                                        |",
           pm[0].mabw1, pm[1].mabw0, pm[1].mabw1);
    printf("\n                           |                   |********************nr_c1***************|");
    printf("\n -pretfs     %3u           |                   |  -nCen              %3u                |",
           pm0->tfs,   p_x->nrc_en);
    printf("\n -premath    %3u           |                   |  -pre_sfs           %3u                |",
           pm0->math,  pc1->pre_sfs);
    printf("\n -premathd   %3u           |                   |  -ncsfs1            %3u                |",
           pm0->mathd, pc1->sfs1);
    printf("\n -premabw    %3u           |                   |  -sfs2_mode         %3u                |",
           pm0->mabw,  pc1->sfs2_mode);
}

static void print_nrx3(const ot_scene_3dnr *p_x)
{
    const t_nr_v2_sfy_lut *psfylut = p_x->luty;
    const t_nr_v2_mdy0 *pm0 = &(p_x->mdy0);
    const t_nr_v2_nrc1 *pc1 = &(p_x->nrc1);

    printf("\n -pretdz     %3u           |                   |  -sfs2_c_f          %3u                |",
           pm0->tdz,  pc1->sfs2_coarse_f);
    printf("\n                           |                   |  -sfs2_c            %3u                |",
           pc1->sfs2_coarse);
    printf("\n                           |                   |  -sfs2_fine_f       %3u                |",
           pc1->sfs2_fine_f);
    printf("\n                           |                   |  -sfs2_fine_b       %3u                |",
           pc1->sfs2_fine_b);
    printf("\n -gamma_en      %3u        |  -ca_en     %3u   |                                        |",
           p_x->pp.gamma_en, p_x->pp.ca_en);
    printf("\n*****************************************************************************************\n");
    printf("\n -n2sf_var_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf_var_f), _t4a_4_(psfylut, 0, sf_var_f), _t4a_8_(psfylut, 0, sf_var_f),
           _t4a_12_(psfylut, 0, sf_var_f), psfylut[0].sf_var_f[0x10]);
    printf("\n -n2sf_var_b   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf_var_b), _t4a_4_(psfylut, 0, sf_var_b), _t4a_8_(psfylut, 0, sf_var_b),
           _t4a_12_(psfylut, 0, sf_var_b), psfylut[0].sf_var_b[0x10]);
    printf("\n -n2sf_bri_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf_bri_f), _t4a_4_(psfylut, 0, sf_bri_f), _t4a_8_(psfylut, 0, sf_bri_f),
           _t4a_12_(psfylut, 0, sf_bri_f), psfylut[0].sf_bri_f[0x10]);
    printf("\n -n2sf_bri_b   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf_bri_b), _t4a_4_(psfylut, 0, sf_bri_b), _t4a_8_(psfylut, 0, sf_bri_b),
           _t4a_12_(psfylut, 0, sf_bri_b), psfylut[0].sf_bri_b[0x10]);
    printf("\n -n2sf_dir_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf_dir_f), _t4a_4_(psfylut, 0, sf_dir_f), _t4a_8_(psfylut, 0, sf_dir_f),
           _t4a_12_(psfylut, 0, sf_dir_f), psfylut[0].sf_dir_f[0x10]);
    printf("\n -n2sf_dir_b   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf_dir_b), _t4a_4_(psfylut, 0, sf_dir_b), _t4a_8_(psfylut, 0, sf_dir_b),
           _t4a_12_(psfylut, 0, sf_dir_b), psfylut[0].sf_dir_b[0x10]);
    printf("\n -n2sf_cor     %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf_cor), _t4a_4_(psfylut, 0, sf_cor), _t4a_8_(psfylut, 0, sf_cor),
           _t4a_12_(psfylut, 0, sf_cor), psfylut[0].sf_cor[0x10]);
    printf("\n -n2sf_mot     %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf_mot), _t4a_4_(psfylut, 0, sf_mot), _t4a_8_(psfylut, 0, sf_mot),
           _t4a_12_(psfylut, 0, sf_mot), psfylut[0].sf_mot[0x10]);
    printf("\n -n2sf5_var_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf5_var_f), _t4a_4_(psfylut, 0, sf5_var_f), _t4a_8_(psfylut, 0, sf5_var_f),
           _t4a_12_(psfylut, 0, sf5_var_f), psfylut[0].sf5_var_f[0x10]);
    printf("\n -n2sf5_var_b   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf5_var_b), _t4a_4_(psfylut, 0, sf5_var_b), _t4a_8_(psfylut, 0, sf5_var_b),
           _t4a_12_(psfylut, 0, sf5_var_b), psfylut[0].sf5_var_b[0x10]);
}

static void print_nrx4(const ot_scene_3dnr *p_x)
{
    const t_nr_v2_sfy_lut *psfylut = p_x->luty;
    printf("\n -n2sf5_bri_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 0, sf5_bri_f), _t4a_4_(psfylut, 0, sf5_bri_f), _t4a_8_(psfylut, 0, sf5_bri_f),
        _t4a_12_(psfylut, 0, sf5_bri_f), psfylut[0].sf5_bri_f[0x10]);
    printf("\n -n2sf5_bri_b   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 0, sf5_bri_b), _t4a_4_(psfylut, 0, sf5_bri_b), _t4a_8_(psfylut, 0, sf5_bri_b),
        _t4a_12_(psfylut, 0, sf5_bri_b), psfylut[0].sf5_bri_b[0x10]);
    printf("\n -n2sf5_dir_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 0, sf5_dir_f), _t4a_4_(psfylut, 0, sf5_dir_f), _t4a_8_(psfylut, 0, sf5_dir_f),
        _t4a_12_(psfylut, 0, sf5_dir_f), psfylut[0].sf5_dir_f[0x10]);
    printf("\n -n2sf5_dir_b   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 0, sf5_dir_b), _t4a_4_(psfylut, 0, sf5_dir_b), _t4a_8_(psfylut, 0, sf5_dir_b),
        _t4a_12_(psfylut, 0, sf5_dir_b), psfylut[0].sf5_dir_b[0x10]);
    printf("\n -n2sf5_sad_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 0, sf5_sad_f), _t4a_4_(psfylut, 0, sf5_sad_f), _t4a_8_(psfylut, 0, sf5_sad_f),
        _t4a_12_(psfylut, 0, sf5_sad_f), psfylut[0].sf5_sad_f[0x10]);
    printf("\n -n2sf5_cor     %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 0, sf5_cor), _t4a_4_(psfylut, 0, sf5_cor), _t4a_8_(psfylut, 0, sf5_cor),
        _t4a_12_(psfylut, 0, sf5_cor), psfylut[0].sf5_cor[0x10]);
    printf("\n -n3sf_var_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 1, sf_var_f), _t4a_4_(psfylut, 1, sf_var_f), _t4a_8_(psfylut, 1, sf_var_f),
        _t4a_12_(psfylut, 1, sf_var_f), psfylut[1].sf_var_f[0x10]);
    printf("\n -n3sf_var_b   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 1, sf_var_b), _t4a_4_(psfylut, 1, sf_var_b), _t4a_8_(psfylut, 1, sf_var_b),
        _t4a_12_(psfylut, 1, sf_var_b), psfylut[1].sf_var_b[0x10]);
    printf("\n -n3sf_bri_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 1, sf_bri_f), _t4a_4_(psfylut, 1, sf_bri_f), _t4a_8_(psfylut, 1, sf_bri_f),
        _t4a_12_(psfylut, 1, sf_bri_f), psfylut[1].sf_bri_f[0x10]);
    printf("\n -n3sf_bri_b   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 1, sf_bri_b), _t4a_4_(psfylut, 1, sf_bri_b), _t4a_8_(psfylut, 1, sf_bri_b),
        _t4a_12_(psfylut, 1, sf_bri_b), psfylut[1].sf_bri_b[0x10]);
    printf("\n -n3sf_dir_f   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 1, sf_dir_f), _t4a_4_(psfylut, 1, sf_dir_f), _t4a_8_(psfylut, 1, sf_dir_f),
        _t4a_12_(psfylut, 1, sf_dir_f), psfylut[1].sf_dir_f[0x10]);
    printf("\n -n3sf_dir_b   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 1, sf_dir_b), _t4a_4_(psfylut, 1, sf_dir_b), _t4a_8_(psfylut, 1, sf_dir_b),
        _t4a_12_(psfylut, 1, sf_dir_b), psfylut[1].sf_dir_b[0x10]);
    printf("\n -n3sf_cor     %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 1, sf_cor), _t4a_4_(psfylut, 1, sf_cor), _t4a_8_(psfylut, 1, sf_cor),
        _t4a_12_(psfylut, 1, sf_cor), psfylut[1].sf_cor[0x10]);
    printf("\n -n3sf_mot     %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(psfylut, 1, sf_mot), _t4a_4_(psfylut, 1, sf_mot), _t4a_8_(psfylut, 1, sf_mot),
        _t4a_12_(psfylut, 1, sf_mot), psfylut[1].sf_mot[0x10]);
}

static void print_nrx5(const ot_scene_3dnr *p_x)
{
    const t_nr_v2_sfy_lut *psfylut = p_x->luty;
    const t_nr_v2_pshrp *pi = &(p_x->iey);

    printf("\n -n2var_by_bri  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, var_by_bri), _t4a_4_(psfylut, 0, var_by_bri), _t4a_8_(psfylut, 0, var_by_bri),
           _t4a_12_(psfylut, 0, var_by_bri), psfylut[0].var_by_bri[0x10]);
    printf("\n -n2sf_bld     %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf_bld), _t4a_4_(psfylut, 0, sf_bld), _t4a_8_(psfylut, 0, sf_bld),
           _t4a_12_(psfylut, 0, sf_bld), psfylut[0].sf_bld[0x10]);
    printf("\n -n3var_by_bri  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 1, var_by_bri), _t4a_4_(psfylut, 1, var_by_bri), _t4a_8_(psfylut, 1, var_by_bri),
           _t4a_12_(psfylut, 1, var_by_bri), psfylut[1].var_by_bri[0x10]);
    printf("\n -n3sf_bld      %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 1, sf_bld), _t4a_4_(psfylut, 1, sf_bld), _t4a_8_(psfylut, 1, sf_bld),
           _t4a_12_(psfylut, 1, sf_bld), psfylut[1].sf_bld[0x10]);
    printf("\n -n4shp_crtl_mean%3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(pi, 0, shp_ctl_mean), _t4a_4_(pi, 0, shp_ctl_mean), _t4a_8_(pi, 0, shp_ctl_mean),
           _t4a_12_(pi, 0, shp_ctl_mean), pi[0].shp_ctl_mean[0x10]);
    printf("\n -n4shp_crtl_dir%3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(pi, 0, shp_ctl_dir), _t4a_4_(pi, 0, shp_ctl_dir), _t4a_8_(pi, 0, shp_ctl_dir),
           _t4a_12_(pi, 0, shp_ctl_dir), pi[0].shp_ctl_dir[0x10]);
    printf("\n -n4sf_bld      %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(pi, 0, sf_bld), _t4a_4_(pi, 0, sf_bld), _t4a_8_(pi, 0, sf_bld), _t4a_12_(pi, 0, sf_bld),
           pi[0].sf_bld[0x10]);
}

static void print_nrx6(const ot_scene_3dnr *p_x)
{
    const t_nr_v2_nrc1 *pc1 = &(p_x->nrc1);
    const t_nr_v2_nrc0 *pc0 = &(p_x->nrc0);
    const t_nr_v2_pp *pp = &(p_x->pp);

    printf("\n -nc0tfs_mot    %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(pc0, 0, tfs_mot), _t4a_4_(pc0, 0, tfs_mot), _t4a_8_(pc0, 0, tfs_mot), _t4a_12_(pc0, 0, tfs_mot),
        pc0[0].tfs_mot[0x10]);

    printf("\n -nc1sfs_mot    %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_0_(pc1, 0, sfs1_mot), _t4a_4_(pc1, 0, sfs1_mot), _t4a_8_(pc1, 0, sfs1_mot), _t4a_12_(pc1, 0, sfs1_mot),
        pc1[0].sfs1_mot[0x10]);
    printf("\n -gamma_lut_0   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u     |",
        _t4a_0_(pp, 0, gamma_lut), _t4a_4_(pp, 0, gamma_lut), _t4a_8_(pp, 0, gamma_lut), _t4a_12_(pp, 0, gamma_lut));
    printf("\n -gamma_lut_1   %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_10_(pp, 0, gamma_lut), _t4a_14_(pp, 0, gamma_lut), _t4a_18_(pp, 0, gamma_lut), _t4a_1c_(pp, 0, gamma_lut),
        pp[0].gamma_lut[0x20]);
    printf("\n -ca_lut_0      %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u     |",
        _t4a_0_(pp, 0, ca_lut), _t4a_4_(pp, 0, ca_lut), _t4a_8_(pp, 0, ca_lut), _t4a_12_(pp, 0, ca_lut));
    printf("\n -ca_lut_1      %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
        _t4a_10_(pp, 0, ca_lut), _t4a_14_(pp, 0, ca_lut), _t4a_18_(pp, 0, ca_lut), _t4a_1c_(pp, 0, ca_lut),
        pp[0].ca_lut[0x20]);
}

static void print_nrx(const ot_scene_3dnr *p_x)
{
    const t_nr_v2_nrc1 *pc1 = &(p_x->nrc1);
    const t_nr_v2_tfy *pt = p_x->tfy;
    const t_nr_v2_mdy0 *pm0 = &(p_x->mdy0);
    const t_nr_v2_sfy_lut *psfylut = p_x->luty;

    print_nrx1(p_x);
    print_nrx2(p_x);
    print_nrx3(p_x);
    print_nrx4(p_x);
    print_nrx5(p_x);
    print_nrx6(p_x);

    printf("\n -n1tfs0_mot    %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(pt, 0, tfs0_mot), _t4a_4_(pt, 0, tfs0_mot), _t4a_8_(pt, 0, tfs0_mot), _t4a_12_(pt, 0, tfs0_mot),
           pt[0].tfs0_mot[0x10]);
    printf("\n -n2tfs0_mot    %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(pt, 1, tfs0_mot), _t4a_4_(pt, 1, tfs0_mot), _t4a_8_(pt, 1, tfs0_mot), _t4a_12_(pt, 1, tfs0_mot),
           pt[1].tfs0_mot[0x10]);
    printf("\n -n1tfs1_mot    %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(pt, 0, tfs1_mot), _t4a_4_(pt, 0, tfs1_mot), _t4a_8_(pt, 0, tfs1_mot), _t4a_12_(pt, 0, tfs1_mot),
           pt[0].tfs1_mot[0x10]);
    printf("\n -n2tfs1_mot    %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(pt, 1, tfs1_mot), _t4a_4_(pt, 1, tfs1_mot), _t4a_8_(pt, 1, tfs1_mot), _t4a_12_(pt, 1, tfs1_mot),
           pt[1].tfs1_mot[0x10]);
    printf("\n -nc1sfs2_mot    %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u |",
           _t4a_0_(pc1, 0, sfs2_mot), _t4a_4_(pc1, 0, sfs2_mot), _t4a_8_(pc1, 0, sfs2_mot), _t4a_12_(pc1, 0, sfs2_mot));
    printf("\n -nc1sfs2_sat    %4u:%4u:%4u:%4u: %4u:%4u:%4u:%4u:  %4u:%4u:%4u:%4u: %4u:%4u:%4u:%4u: %4u:%4u:%4u:%4u |",
           _t4a_0_(pc1, 0, sfs2_sat), _t4a_4_(pc1, 0, sfs2_sat), _t4a_8_(pc1, 0, sfs2_sat), _t4a_12_(pc1, 0, sfs2_sat),
           _t4a_10_(pc1, 0, sfs2_sat));

    printf("\n -n0tfs_mot    %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(pm0, 0, tfs_mot), _t4a_4_(pm0, 0, tfs_mot), _t4a_8_(pm0, 0, tfs_mot), _t4a_12_(pm0, 0, tfs_mot),
           pm0[0].tfs_mot[0x10]);

    printf("\n -n2sf5_mot     %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:  %3u:%3u:%3u:%3u: %3u:%3u:%3u:%3u:%3u |",
           _t4a_0_(psfylut, 0, sf5_mot), _t4a_4_(psfylut, 0, sf5_mot), _t4a_8_(psfylut, 0, sf5_mot),
           _t4a_12_(psfylut, 0, sf5_mot), psfylut[0].sf5_mot[0x10]);

    printf("\n");
}

td_void scene_set_static_h265_avbr(ot_venc_rc_param *rc_param, td_u8 index)
{
    rc_param->h265_avbr_param.chg_pos = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.change_pos;
    rc_param->h265_avbr_param.min_i_proportion =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.min_iprop;
    rc_param->h265_avbr_param.max_i_proportion =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.max_iprop;
    rc_param->h265_avbr_param.max_reencode_times =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.max_reencode_times;
    rc_param->h265_avbr_param.min_still_percent =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.min_still_percent;
    rc_param->h265_avbr_param.max_still_qp =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.max_still_qp;
    rc_param->h265_avbr_param.min_still_psnr =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.min_still_psnr;
    rc_param->h265_avbr_param.max_qp = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.max_qp;
    rc_param->h265_avbr_param.min_qp = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.min_qp;
    rc_param->h265_avbr_param.max_i_qp = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.max_iqp;
    rc_param->h265_avbr_param.min_i_qp = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.min_iqp;
    rc_param->h265_avbr_param.min_qp_delta =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.min_qp_delta;
    rc_param->h265_avbr_param.motion_sensitivity =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.motion_sensitivity;
    rc_param->h265_avbr_param.qpmap_en = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.qp_map_en;
    rc_param->h265_avbr_param.qpmap_mode = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_avbr.qp_map_mode;
}

td_void scene_set_static_h265_cvbr(ot_venc_rc_param *rc_param, td_u8 index)
{
    rc_param->h264_cbr_param.min_i_proportion =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.min_iprop;
    rc_param->h265_cvbr_param.max_i_proportion =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.max_iprop;
    rc_param->h265_cvbr_param.max_reencode_times =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.max_reencode_times;
    rc_param->h265_cvbr_param.max_qp = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.max_qp;
    rc_param->h265_cvbr_param.min_qp = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.min_qp;
    rc_param->h265_cvbr_param.max_i_qp = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.max_iqp;
    rc_param->h265_cvbr_param.min_i_qp = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.min_iqp;
    rc_param->h265_cvbr_param.min_qp_delta =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.min_qp_delta;
    rc_param->h265_cvbr_param.qpmap_en = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.qp_map_en;
    rc_param->h265_cvbr_param.qpmap_mode = g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.qp_map_mode;
    rc_param->h265_cvbr_param.max_qp_delta =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.max_qp_delta;
    rc_param->h265_cvbr_param.extra_bit_percent =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.extra_bit_percent;
    rc_param->h265_cvbr_param.long_term_stats_time_unit =
        g_scene_pipe_param[index].static_venc_attr.venc_param_h265_cvbr.long_term_static_time_unit;
}

td_void scene_set_isp_attr(set_isp_attr_param param, const td_u32 *ratio_level_thresh, const td_u32 *iso_level_thresh,
    ot_isp_drc_attr *isp_drc_attr)
{
    struct iso_context ictx = {g_scene_pipe_param, param.index, param.iso, param.iso_level,
        param.iso_count, iso_level_thresh};
    struct ratio_context rctx = {g_scene_pipe_param, param.index, param.wdr_ratio, param.ratio_level,
        param.ratio_count, ratio_level_thresh};
    td_u16 drc_str_incre_tmp;
    drc_str_incre_tmp = 0;

    for (td_u32 i = 0; i < OT_ISP_DRC_LMIX_NODE_NUM; i++) {
        scene_set_drc_member(isp_drc_attr->color_correction_lut[i], color_correction_lut[i], 0, ictx, rctx);
    }

    scene_set_drc_member(isp_drc_attr->spatial_filter_coef, spatial_filter_coef, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->range_filter_coef, range_filter_coef, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->detail_adjust_coef, detail_adjust_coef, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->rim_reduction_strength, rim_reduction_strength, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->rim_reduction_threshold, rim_reduction_threshold, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->bright_gain_limit, bright_gain_limit, 0, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->bright_gain_limit_step, bright_gain_limit_step, 0, ictx, rctx);
    scene_set_drc_member(drc_str_incre_tmp, drc_str_incre, 0, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->dark_gain_limit_luma, dark_gain_limit_luma, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->global_color_ctrl, global_color_ctrl, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->shoot_reduction_en, shoot_reduction_en, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->dark_gain_limit_chroma, dark_gain_limit_chroma, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->local_mixing_bright_param.max, local_mixing_bright_max, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->local_mixing_bright_param.min, local_mixing_bright_min, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->local_mixing_bright_param.threshold, local_mixing_bright_threshold, 1,
        ictx, rctx);
    scene_set_drc_member_minus(isp_drc_attr->local_mixing_bright_param.slope, local_mixing_bright_slope, 1,
        ictx, rctx);

    scene_set_drc_member(isp_drc_attr->local_mixing_dark_param.max, local_mixing_dark_max, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->local_mixing_dark_param.min, local_mixing_dark_min, 1, ictx, rctx);
    scene_set_drc_member(isp_drc_attr->local_mixing_dark_param.threshold, local_mixing_dark_threshold, 1, ictx, rctx);
    scene_set_drc_member_minus(isp_drc_attr->local_mixing_dark_param.slope, local_mixing_dark_slope, 1, ictx, rctx);

    if (isp_drc_attr->op_type == 0) {
        scene_set_drc_member(isp_drc_attr->auto_attr.strength, manual_str, 1, ictx, rctx);
        isp_drc_attr->auto_attr.strength = isp_drc_attr->auto_attr.strength + drc_str_incre_tmp;
        isp_drc_attr->auto_attr.strength = clip3(isp_drc_attr->auto_attr.strength, 0, 1023); /* max 1023 */
    } else {
        scene_set_drc_member(isp_drc_attr->manual_attr.strength, manual_str, 1, ictx, rctx);
        isp_drc_attr->manual_attr.strength = isp_drc_attr->manual_attr.strength + drc_str_incre_tmp;
        isp_drc_attr->manual_attr.strength = clip3(isp_drc_attr->manual_attr.strength, 0, 1023); /* max 1023 */
    }
}


td_s32 scene_blend_tone_mapping(ot_vi_pipe vi_pipe, td_u8 index, ot_scene_dynamic_drc_coef *drc_coef,
    ot_isp_drc_attr *isp_drc_attr)
{
    td_u32 i, j;
    td_u16 tonemapping_value[OT_ISP_DRC_TM_NODE_NUM] = {0};
    ot_isp_exp_info ae_exp_info;
    td_s32 ret;
    td_u16 blend_alpha;

    check_scene_return_if_pause();
    ret = ss_mpi_isp_query_exposure_info(vi_pipe, &ae_exp_info);
    check_scene_ret(ret);

    blend_alpha = (ae_exp_info.wdr_exp_coef < 0x200) ? 0 : ae_exp_info.wdr_exp_coef - 0x200; /* clip 0x200 */

    if (drc_coef->ref_exp_ratio >= (g_scene_pipe_param[index].static_wdr_exposure.exp_ratio_max - 2048)) { /* sub2048 */
        for (i = 0; i < OT_ISP_DRC_TM_NODE_NUM; i++) {
            tonemapping_value[i] =
                ((td_u32)(0x200 - blend_alpha) * g_scene_pipe_param[index].dynamic_drc.tm_val_higher[i] +
                /* 0x200 blend_alpha, precison 9bit */
                (td_u32)(blend_alpha) * g_scene_pipe_param[index].dynamic_drc.tm_val_highest[i]) >> 9; /* shift9 */
        }
    } else if (drc_coef->ref_exp_ratio <= g_scene_pipe_param[index].dynamic_drc.tm_ratio_threshold) {
        for (i = 0; i < OT_ISP_DRC_TM_NODE_NUM; i++) {
            tonemapping_value[i] =
                ((td_u32)drc_coef->ref_ratio_alpha * g_scene_pipe_param[index].dynamic_drc.tm_value_low[i] +
                /* 0x100 ref_exp_ratio_alpha, precison 8bit */
                (td_u32)(0x100 - drc_coef->ref_ratio_alpha) * g_scene_pipe_param[index].dynamic_drc.tm_value_high[i]) >>
                8; /* shift8 */
        }
    } else {
        for (i = 0; i < OT_ISP_DRC_TM_NODE_NUM; i++) {
            tonemapping_value[i] =
                /* 0x100 ref_exp_ratio_alpha, precison 8bit */
                ((td_u32)(0x100 - drc_coef->ref_ratio_alpha) * g_scene_pipe_param[index].dynamic_drc.tm_value_high[i] +
                (td_u32)drc_coef->ref_ratio_alpha * g_scene_pipe_param[index].dynamic_drc.tm_val_higher[i]) >>
                8; /* sf8 */
        }
    }

    for (j = 0; j < OT_ISP_DRC_TM_NODE_NUM; j++) {
        isp_drc_attr->tone_mapping_value[j] = scene_time_filter(isp_drc_attr->tone_mapping_value[j],
            tonemapping_value[j], g_scene_pipe_param[index].dynamic_drc.interval, drc_coef->k);
    }

    return TD_SUCCESS;
}

td_s32 scene_set_tone_mapping_value(ot_vi_pipe vi_pipe, td_u8 index, td_u32 k,
    const ot_isp_inner_state_info *inner_state_info, ot_isp_drc_attr *isp_drc_attr)
{
    static td_u32 ref_exp_ratio[OT_VI_MAX_PIPE_NUM][OT_SCENE_REF_EXP_RATIO_FRM] = {0};
    static td_u32 actual_exp_ratio[OT_VI_MAX_PIPE_NUM][OT_SCENE_REF_EXP_RATIO_FRM] = {0};
    td_u32 cur_ref_exp_ratio = 0;
    td_u32 cur_actual_exp_ratio = 0;
    td_u32 ref_ratio_level;
    td_u32 ref_exp_ratio_alpha;
    ot_isp_exp_info isp_exp_info;
    td_u32 i;
    ot_scene_dynamic_drc_coef dynamic_drc_coef = { 0 };
    td_s32 ret;

    ot_scenecomm_expr_true_return(index >= OT_SCENE_PIPETYPE_NUM, TD_FAILURE);
    isp_drc_attr->enable = g_scene_pipe_param[index].dynamic_drc.enable;
    ot_scenecomm_expr_true_return(vi_pipe >= OT_VI_MAX_PIPE_NUM, TD_FAILURE);

    ret = ss_mpi_isp_query_exposure_info(vi_pipe, &isp_exp_info);
    check_scene_ret(ret);

    for (i = 0; i < OT_SCENE_REF_EXP_RATIO_FRM - 1; i++) {
        ref_exp_ratio[vi_pipe][i] = ref_exp_ratio[vi_pipe][i + 1];
        actual_exp_ratio[vi_pipe][i] = actual_exp_ratio[vi_pipe][i + 1];
    }
    ref_exp_ratio[vi_pipe][i] = isp_exp_info.ref_exp_ratio;
    actual_exp_ratio[vi_pipe][i] = inner_state_info->wdr_exp_ratio_actual[0];

    for (i = 0; i < OT_SCENE_REF_EXP_RATIO_FRM; i++) {
        cur_ref_exp_ratio += ref_exp_ratio[vi_pipe][i];
        cur_actual_exp_ratio += actual_exp_ratio[vi_pipe][i];
    }

    cur_ref_exp_ratio = cur_ref_exp_ratio / OT_SCENE_REF_EXP_RATIO_FRM;
    cur_actual_exp_ratio = cur_actual_exp_ratio / OT_SCENE_REF_EXP_RATIO_FRM;

    cur_ref_exp_ratio = cur_actual_exp_ratio;

    ref_ratio_level = scene_get_level_ltoh_u32(cur_ref_exp_ratio, g_scene_pipe_param[index].dynamic_drc.ref_ratio_count,
        g_scene_pipe_param[index].dynamic_drc.ref_ratio_ltoh, OT_SCENE_DRC_REF_RATIO_MAX_COUNT);

    ot_scenecomm_expr_true_return(ref_ratio_level >= OT_SCENE_DRC_REF_RATIO_MAX_COUNT, TD_FAILURE);
    if ((ref_ratio_level == 0) || (ref_ratio_level == (g_scene_pipe_param[index].dynamic_drc.ref_ratio_count - 1))) {
        ref_exp_ratio_alpha = g_scene_pipe_param[index].dynamic_drc.ref_ratio_alpha[ref_ratio_level];
    } else {
        ref_exp_ratio_alpha = scene_interpulate(cur_ref_exp_ratio,
            g_scene_pipe_param[index].dynamic_drc.ref_ratio_ltoh[ref_ratio_level - 1],
            g_scene_pipe_param[index].dynamic_drc.ref_ratio_alpha[ref_ratio_level - 1],
            g_scene_pipe_param[index].dynamic_drc.ref_ratio_ltoh[ref_ratio_level],
            g_scene_pipe_param[index].dynamic_drc.ref_ratio_alpha[ref_ratio_level]);
    }

    dynamic_drc_coef.ref_exp_ratio = cur_ref_exp_ratio;
    dynamic_drc_coef.ref_ratio_alpha = ref_exp_ratio_alpha;
    dynamic_drc_coef.k = k;
    ret = scene_blend_tone_mapping(vi_pipe, index, &dynamic_drc_coef, isp_drc_attr);
    check_scene_ret(ret);

    return TD_SUCCESS;
}

td_s32 scene_set_isp_drc_attr(td_u8 index, td_u64 iso, ot_isp_drc_attr *isp_drc_attr)
{
    ot_scenecomm_expr_true_return(index >= OT_SCENE_PIPETYPE_NUM, TD_FAILURE);
    struct iso_context ctx = {0};
    ctx.pipe_params = &g_scene_pipe_param[0];
    ctx.index = index;
    ctx.count = g_scene_pipe_param[index].dynamic_linear_drc.iso_count;
    ctx.table = g_scene_pipe_param[index].dynamic_linear_drc.iso_level;
    isp_drc_attr->enable = g_scene_pipe_param[index].dynamic_linear_drc.enable;
    ctx.level = (td_u32)scene_get_level_ltoh_u32((td_u32)iso, ctx.count, ctx.table, OT_SCENE_DRC_ISO_MAX_COUNT);
    ot_scenecomm_expr_true_return(ctx.level >= OT_SCENE_DRC_ISO_MAX_COUNT, TD_FAILURE);

    scene_set_dynamic_linear_drc(isp_drc_attr->spatial_filter_coef, spatial_filter_coef, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->range_filter_coef, range_filter_coef, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->detail_adjust_coef, detail_adjust_coef, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->rim_reduction_strength, rim_reduction_strength, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->rim_reduction_threshold, rim_reduction_threshold, ctx);

    scene_set_dynamic_linear_drc(isp_drc_attr->bright_gain_limit, bright_gain_limit, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->bright_gain_limit_step, bright_gain_limit_step, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->dark_gain_limit_luma, dark_gain_limit_luma, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->dark_gain_limit_chroma, dark_gain_limit_chroma, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->high_saturation_color_ctrl, high_saturation_color_ctrl, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->global_color_ctrl, global_color_ctrl, ctx);

    scene_set_dynamic_linear_drc(isp_drc_attr->local_mixing_bright_param.max, local_mixing_bright_max, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->local_mixing_bright_param.min, local_mixing_bright_min, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->local_mixing_bright_param.threshold, local_mixing_bright_threshold,
        ctx);
    scene_set_dynamic_linear_drc_signed(isp_drc_attr->local_mixing_bright_param.slope, local_mixing_bright_slope, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->local_mixing_dark_param.max, local_mixing_dark_max, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->local_mixing_dark_param.min, local_mixing_dark_min, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->local_mixing_dark_param.threshold, local_mixing_dark_threshold, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->auto_curve.brightness, curve_brightness, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->auto_curve.contrast, curve_contrast, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->auto_curve.tolerance, curve_tolerance, ctx);
    scene_set_dynamic_linear_drc_signed(isp_drc_attr->local_mixing_dark_param.slope, local_mixing_dark_slope, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->bcnr_attr.strength, drc_bcnr_str, ctx);

    for (td_u8 i = 0; i < OT_ISP_DRC_BCNR_NODE_NUM; i++) {
        scene_set_dynamic_linear_drc(isp_drc_attr->bcnr_attr.detail_restore_lut[i], detail_restore_lut[i], ctx);
    }
    scene_set_dynamic_linear_drc(isp_drc_attr->asymmetry_curve.asymmetry, asymmetry, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->asymmetry_curve.second_pole, second_pole, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->asymmetry_curve.compress, compress, ctx);
    scene_set_dynamic_linear_drc(isp_drc_attr->asymmetry_curve.stretch, stretch, ctx);

    if (isp_drc_attr->op_type == 0) {
        scene_set_dynamic_linear_drc(isp_drc_attr->auto_attr.strength, strength, ctx);
    } else {
        scene_set_dynamic_linear_drc(isp_drc_attr->manual_attr.strength, strength, ctx);
    }
    return TD_SUCCESS;
}


static td_void scene_set_wdr_cfg(td_u8 index, td_u32 iso_level, ot_isp_nr_attr *nr_attr,
    const ot_scene_nr_para *nr_para)
{
    td_s32 i;

    for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        nr_attr->wdr_cfg.snr_sfm0_wdr_strength[i] =
            g_scene_pipe_param[index].dynamic_nr.snr_sfm0_wdr_frame_str[i][iso_level] + nr_para->sfm0_wdr_incr[i];
        nr_attr->wdr_cfg.snr_sfm0_fusion_strength[i] =
            g_scene_pipe_param[index].dynamic_nr.snr_sfm0_fusion_frame_str[i][iso_level] + nr_para->sfm0_fusion_incr[i];
    }
}

static td_void scene_set_nr_part1(td_u8 index, td_u32 iso, ot_isp_nr_attr *nr_attr, const ot_scene_nr_para *nr_para)
{
    td_s32 i;
    td_u32 count = g_scene_pipe_param[index].dynamic_nr.coring_ratio_count;
    td_u32 *pthresh = (td_u32 *)g_scene_pipe_param[index].dynamic_nr.coring_ratio_iso;
    td_u32 iso_level = scene_get_level_ltoh_u32(iso, count, pthresh, OT_SCENE_NR_RATIO_MAX_COUNT);

    td_u32 mid_iso = (td_u32)iso;
    td_u32 left_iso = pthresh[iso_level - 1];
    td_u32 right_iso = pthresh[iso_level];

    for (i = 0; i < OT_ISP_WDR_MAX_FRAME_NUM; i++) {
        nr_attr->wdr_cfg.snr_sfm0_wdr_strength[i] = scene_interpulate(mid_iso, left_iso,
            g_scene_pipe_param[index].dynamic_nr.snr_sfm0_wdr_frame_str[i][iso_level - 1], right_iso,
            g_scene_pipe_param[index].dynamic_nr.snr_sfm0_wdr_frame_str[i][iso_level]) +
            nr_para->sfm0_wdr_incr[i];
        nr_attr->wdr_cfg.snr_sfm0_fusion_strength[i] = scene_interpulate(mid_iso, left_iso,
            g_scene_pipe_param[index].dynamic_nr.snr_sfm0_fusion_frame_str[i][iso_level - 1], right_iso,
            g_scene_pipe_param[index].dynamic_nr.snr_sfm0_fusion_frame_str[i][iso_level]) +
            nr_para->sfm0_fusion_incr[i];
    }
}

td_s32 scene_set_nr_attr_para(td_u8 index, td_u32 iso, ot_isp_nr_attr *nr_attr, const ot_scene_nr_para *nr_para)
{
    td_u32 count = g_scene_pipe_param[index].dynamic_nr.coring_ratio_count;
    td_u32 *pthresh = (td_u32 *)g_scene_pipe_param[index].dynamic_nr.coring_ratio_iso;
    td_u32 iso_level = scene_get_level_ltoh_u32(iso, count, pthresh, OT_SCENE_NR_RATIO_MAX_COUNT);
    ot_scenecomm_expr_true_return(iso_level >= OT_SCENE_ISO_STRENGTH_NUM, TD_FAILURE);

    if ((iso_level == (count - 1)) || (iso_level == 0)) {
        scene_set_wdr_cfg(index, iso_level, nr_attr, nr_para);
    } else {
        scene_set_nr_part1(index, iso, nr_attr, nr_para);
    }

    return TD_SUCCESS;
}

td_s32 scene_set_nr_wdr_ratio_para(ot_vi_pipe vi_pipe, td_u8 index, td_u32 wdr_ratio, td_u32 ratio_index,
    ot_isp_nr_attr *nr_attr)
{
    td_u32 i;
    td_s32 ret;
    ot_isp_pub_attr pub_attr;

    ret = ss_mpi_isp_get_pub_attr(vi_pipe, &pub_attr);
    check_scene_ret(ret);

    if (pub_attr.wdr_mode == OT_WDR_MODE_NONE) {
        return TD_SUCCESS;
    }

    if (wdr_ratio > get_pipe_params()[index].dynamic_nr.wdr_ratio_threshold) {
        for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
            nr_attr->snr_cfg.snr_attr.snr_auto.fine_strength[i] =
                get_pipe_params()[index].dynamic_nr.fine_strength_h[i];
        }
    } else {
        for (i = 0; i < OT_ISP_AUTO_ISO_NUM; i++) {
            nr_attr->snr_cfg.snr_attr.snr_auto.fine_strength[i] =
                get_pipe_params()[index].dynamic_nr.fine_strength_l[i];
        }
    }
    return TD_SUCCESS;
}

static td_void scene_set_nry_en(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nry1_en = _3dnr->nry1_en;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nry2_en = _3dnr->nry2_en;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nry3_en = _3dnr->nry3_en;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nry4_en = _3dnr->nry4_en;

    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.pp.gamma_en = _3dnr->pp.gamma_en;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.pp.ca_en = _3dnr->pp.ca_en;

    for (td_u32 i = 0; i < 33; i++) { /*  33 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.pp.gamma_lut[i] = _3dnr->pp.gamma_lut[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.pp.ca_lut[i] = _3dnr->pp.ca_lut[i];
    }
}

static td_void scene_set_nry_iey(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.coarse_g1 = _3dnr->iey.coarse_g1;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.coarse_g2 = _3dnr->iey.coarse_g2;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.coarse_g3 = _3dnr->iey.coarse_g3;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.coarse_g4 = _3dnr->iey.coarse_g4;

    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.fine_g1 = _3dnr->iey.fine_g1;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.fine_g2 = _3dnr->iey.fine_g2;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.fine_g3 = _3dnr->iey.fine_g3;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.fine_g4 = _3dnr->iey.fine_g4;

    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.o_sht_b = _3dnr->iey.o_sht_b;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.u_sht_b = _3dnr->iey.u_sht_b;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.o_sht_f = _3dnr->iey.o_sht_f;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.u_sht_f = _3dnr->iey.u_sht_f;

    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn7_0 = _3dnr->iey.sfn7_0;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn7_1 = _3dnr->iey.sfn7_1;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn8_0 = _3dnr->iey.sfn8_0;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn8_1 = _3dnr->iey.sfn8_1;

    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sth1_0 = _3dnr->iey.sth1_0;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sth2_0 = _3dnr->iey.sth2_0;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sth3_0 = _3dnr->iey.sth3_0;

    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sth1_1 = _3dnr->iey.sth1_1;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sth2_1 = _3dnr->iey.sth2_1;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sth3_1 = _3dnr->iey.sth3_1;

    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn0_0 = _3dnr->iey.sfn0_0;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn1_0 = _3dnr->iey.sfn1_0;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn2_0 = _3dnr->iey.sfn2_0;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn3_0 = _3dnr->iey.sfn3_0;

    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn0_1 = _3dnr->iey.sfn0_1;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn1_1 = _3dnr->iey.sfn1_1;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn2_1 = _3dnr->iey.sfn2_1;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sfn3_1 = _3dnr->iey.sfn3_1;

    for (td_u32 i = 0; i < 17; i++) { /*  17 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.sf_bld[i] = _3dnr->iey.sf_bld[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.shp_ctl_dir[i] = _3dnr->iey.shp_ctl_dir[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.iey.shp_ctl_mean[i] = _3dnr->iey.shp_ctl_mean[i];
    }
}

static td_void scene_set_nry_sfy_1to5(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    /* sf_y */
    for (td_u32 i = 0; i < 3; i++) { /* array sfy have 3 valid element */
        /* sfs1 */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfs1 = _3dnr->sfy[i].sfs1;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sbr1 = _3dnr->sfy[i].sbr1;

        /* sfs2 */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfs2 = _3dnr->sfy[i].sfs2;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sft2 = _3dnr->sfy[i].sft2;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sbr2 = _3dnr->sfy[i].sbr2;

        /* sfs4 */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfs4 = _3dnr->sfy[i].sfs4;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sft4 = _3dnr->sfy[i].sft4;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sbr4 = _3dnr->sfy[i].sbr4;
    }

    /* sfs3 and sfk4 */
    for (td_u32 i = 1; i < 3; i++) { /* array sfy have 3 valid element */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].strf3 = _3dnr->sfy[i].strf3;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].strb3 = _3dnr->sfy[i].strb3;

        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].strf4 = _3dnr->sfy[i].strf4;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].strb4 = _3dnr->sfy[i].strb4;
    }
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[1].sfs5 = _3dnr->sfy[1].sfs5; /* 2 index */
}

static td_void scene_set_nry_sfy_6to8(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    for (td_u32 i = 0; i < 3; i++) { /*  3 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn6_0 = _3dnr->sfy[i].sfn6_0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn6_1 = _3dnr->sfy[i].sfn6_1;

        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn0_0 = _3dnr->sfy[i].sfn0_0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn1_0 = _3dnr->sfy[i].sfn1_0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn2_0 = _3dnr->sfy[i].sfn2_0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn3_0 = _3dnr->sfy[i].sfn3_0;

        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn0_1 = _3dnr->sfy[i].sfn0_1;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn1_1 = _3dnr->sfy[i].sfn1_1;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn2_1 = _3dnr->sfy[i].sfn2_1;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn3_1 = _3dnr->sfy[i].sfn3_1;

        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sth1_0 = _3dnr->sfy[i].sth1_0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sth2_0 = _3dnr->sfy[i].sth2_0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sth3_0 = _3dnr->sfy[i].sth3_0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sth1_1 = _3dnr->sfy[i].sth1_1;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sth2_1 = _3dnr->sfy[i].sth2_1;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sth3_1 = _3dnr->sfy[i].sth3_1;
    }

    for (td_u32 i = 1; i <= 2; i++) { /* 2 is index */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn7_0 = _3dnr->sfy[i].sfn7_0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn7_1 = _3dnr->sfy[i].sfn7_1;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn8_0 = _3dnr->sfy[i].sfn8_0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[i].sfn8_1 = _3dnr->sfy[i].sfn8_1;
    }
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.sfy[0].bld6 = _3dnr->sfy[0].bld6;
}

static td_void scene_set_nry2_sfy_lut(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    for (td_u32 i = 0; i < 17; i++) { /*  17 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf_var_f[i] = _3dnr->luty[0].sf_var_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf_var_b[i] = _3dnr->luty[0].sf_var_b[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf_dir_f[i] = _3dnr->luty[0].sf_dir_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf_dir_b[i] = _3dnr->luty[0].sf_dir_b[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf_bri_f[i] = _3dnr->luty[0].sf_bri_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf_bri_b[i] = _3dnr->luty[0].sf_bri_b[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf_cor[i] = _3dnr->luty[0].sf_cor[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf_mot[i] = _3dnr->luty[0].sf_mot[i];

        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf5_var_f[i] = _3dnr->luty[0].sf5_var_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf5_var_b[i] = _3dnr->luty[0].sf5_var_b[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf5_dir_f[i] = _3dnr->luty[0].sf5_dir_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf5_dir_b[i] = _3dnr->luty[0].sf5_dir_b[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf5_bri_f[i] = _3dnr->luty[0].sf5_bri_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf5_bri_b[i] = _3dnr->luty[0].sf5_bri_b[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf5_cor[i] = _3dnr->luty[0].sf5_cor[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf5_sad_f[i] = _3dnr->luty[0].sf5_sad_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf5_mot[i] = _3dnr->luty[0].sf5_mot[i];

        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].var_by_bri[i] = _3dnr->luty[0].var_by_bri[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[0].sf_bld[i] = _3dnr->luty[0].sf_bld[i];
    }
}

static td_void scene_set_nry3_sfy_lut(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    for (td_u32 i = 0; i < 17; i++) { /*  17 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].sf_var_f[i] = _3dnr->luty[1].sf_var_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].sf_var_b[i] = _3dnr->luty[1].sf_var_b[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].sf_dir_f[i] = _3dnr->luty[1].sf_dir_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].sf_dir_b[i] = _3dnr->luty[1].sf_dir_b[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].sf_bri_f[i] = _3dnr->luty[1].sf_bri_f[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].sf_bri_b[i] = _3dnr->luty[1].sf_bri_b[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].sf_cor[i] = _3dnr->luty[1].sf_cor[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].sf_mot[i] = _3dnr->luty[1].sf_mot[i];

        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].var_by_bri[i] = _3dnr->luty[1].var_by_bri[i];
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.luty[1].sf_bld[i] = _3dnr->luty[1].sf_bld[i];
    }
}

static td_void scene_set_nry_tfy(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    /* ref */
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[1].ref_en = _3dnr->tfy[1].ref_en;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[0].tfs_mode = _3dnr->tfy[0].tfs_mode;

    /* tf_y */
    for (td_u32 i = 0; i < scene_array_size(_3dnr->tfy); i++) {
        /* tss */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tss0 = _3dnr->tfy[i].tss0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tss1 = _3dnr->tfy[i].tss1;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tss2 = _3dnr->tfy[i].tss2;

        /* tfs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tfs0 = _3dnr->tfy[i].tfs0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tfs1 = _3dnr->tfy[i].tfs1;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tfs2 = _3dnr->tfy[i].tfs2;
    }

    /* tfr and tfs_mot */
    for (td_u32 i = 0; i < scene_array_size(_3dnr->tfy); i++) {
        /* tfr and tfs_mot */
        for (td_u32 j = 0; j < 6; j++) { /*  6 indexs */
            nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tfr0[j] = _3dnr->tfy[i].tfr0[j];
            nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tfr1[j] = _3dnr->tfy[i].tfr1[j];
        }
        for (td_u32 j = 0; j < 17; j++) { /*  17 indexs */
            nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tfs0_mot[j] = _3dnr->tfy[i].tfs0_mot[j];
            nrx_attr->nr_norm_param_v2.nr_manual.nr_param.tfy[i].tfs1_mot[j] = _3dnr->tfy[i].tfs1_mot[j];
        }
    }
}

static td_void scene_set_nry_mdy(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    /* md_y */
    for (td_u32 i = 0; i < scene_array_size(_3dnr->mdy); i++) {
        /* math */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy[i].math0 = _3dnr->mdy[i].math0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy[i].math1 = _3dnr->mdy[i].math1;

        /* mate */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy[i].mate0 = _3dnr->mdy[i].mate0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy[i].mate1 = _3dnr->mdy[i].mate1;

        /* mabw   mdy[0].mabw0 use default value */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy[1].mabw0 = _3dnr->mdy[1].mabw0;
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy[i].mabw1 = _3dnr->mdy[i].mabw1;
    }
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy0.tfs = _3dnr->mdy0.tfs;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy0.math = _3dnr->mdy0.math;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy0.mathd = _3dnr->mdy0.mathd;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy0.mabw = _3dnr->mdy0.mabw;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy0.tdz = _3dnr->mdy0.tdz;

    for (td_u32 i = 0; i < 17; i++) { /*  17 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.mdy0.tfs_mot[i] = _3dnr->mdy0.tfs_mot[i];
    }
}

static td_void scene_set_nry_nc0(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    /* nrc0_en */
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc0_mode = _3dnr->nrc0_mode;

    /* tfs */
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc0.tfs = _3dnr->nrc0.tfs;

    /* trc */
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc0.trc = _3dnr->nrc0.trc;

    /* trc */
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc0.tfc = _3dnr->nrc0.tfc;

    /* sfc */
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc0.sfc = _3dnr->nrc0.sfc;

    for (td_u32 i = 0; i < 17; i++) { /*  17 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc0.tfs_mot[i] = _3dnr->nrc0.tfs_mot[i];
    }
}

static td_void scene_set_nry_nc1(ot_3dnr_param *nrx_attr, const ot_scene_3dnr *_3dnr)
{
    /* nrc1_en */
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc_en = _3dnr->nrc_en;

    /* sfs2 */
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.pre_sfs = _3dnr->nrc1.pre_sfs;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.sfs1 = _3dnr->nrc1.sfs1;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.sfs2_mode = _3dnr->nrc1.sfs2_mode;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.sfs2_coarse_f = _3dnr->nrc1.sfs2_coarse_f;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.sfs2_coarse = _3dnr->nrc1.sfs2_coarse;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.sfs2_fine_f = _3dnr->nrc1.sfs2_fine_f;
    nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.sfs2_fine_b = _3dnr->nrc1.sfs2_fine_b;

    for (td_u32 i = 0; i < 17; i++) { /*  17 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.sfs1_mot[i] = _3dnr->nrc1.sfs1_mot[i];
    }
    for (td_u32 i = 0; i < 16; i++) { /*  16 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.sfs2_mot[i] = _3dnr->nrc1.sfs2_mot[i];
    }
    for (td_u32 i = 0; i < 20; i++) { /*  20 indexs */
        nrx_attr->nr_norm_param_v2.nr_manual.nr_param.nrc1.sfs2_sat[i] = _3dnr->nrc1.sfs2_sat[i];
    }
}

td_s32 scene_set_3dnr(ot_vi_pipe vi_pipe, const ot_scene_3dnr *_3dnr, td_u8 index)
{
    td_s32 ret;

    ot_3dnr_param nrx_attr, nrx_get_attr;

    nrx_attr.nr_version = OT_NR_V2;
    nrx_attr.nr_norm_param_v2.op_mode = OT_OP_MODE_MANUAL;
    ret = ss_mpi_vi_get_pipe_3dnr_param(vi_pipe, &nrx_attr);
    check_scene_ret(ret);

    scene_set_nry_en(&nrx_attr, _3dnr);
    scene_set_nry_iey(&nrx_attr, _3dnr);
    scene_set_nry_sfy_1to5(&nrx_attr, _3dnr);
    scene_set_nry_sfy_6to8(&nrx_attr, _3dnr);
    scene_set_nry2_sfy_lut(&nrx_attr, _3dnr);
    scene_set_nry3_sfy_lut(&nrx_attr, _3dnr);

    scene_set_nry_tfy(&nrx_attr, _3dnr);
    scene_set_nry_mdy(&nrx_attr, _3dnr);

    scene_set_nry_nc0(&nrx_attr, _3dnr);
    scene_set_nry_nc1(&nrx_attr, _3dnr);

    nrx_attr.nr_version = OT_NR_V2;
    nrx_attr.nr_norm_param_v2.op_mode = OT_OP_MODE_MANUAL;
    ret = ss_mpi_vi_set_pipe_3dnr_param(vi_pipe, &nrx_attr);
    check_scene_ret(ret);

    nrx_get_attr.nr_version = OT_NR_V2;
    nrx_get_attr.nr_norm_param_v2.op_mode = OT_OP_MODE_MANUAL;
    ret = ss_mpi_vi_get_pipe_3dnr_param(vi_pipe, &nrx_get_attr);
    check_scene_ret(ret);

    static td_u32 count[OT_VI_MAX_PIPE_NUM] = {0};
    if ((g_scene_pipe_param[index].module_state.debug == 1) && ((count[vi_pipe] % 15) == 0)) { /* 15 max count */
        printf("2======================nrx_get_attr ================================");
        print_nrx(_3dnr);
    }
    count[vi_pipe]++;
    return TD_SUCCESS;
}

td_void scene_set_3dnr_nrx_nry(const _3dnr_nrx_pack *pack)
{
    scene_set_nrx_attr_interpulate(pack, nry1_en);
    scene_set_nrx_attr_interpulate(pack, nry2_en);
    scene_set_nrx_attr_interpulate(pack, nry3_en);
    scene_set_nrx_attr_interpulate(pack, nry4_en);

    scene_set_nrx_attr_interpulate(pack, b_delay_mode);
    scene_set_nrx_attr_interpulate(pack, pp.gamma_en);
    scene_set_nrx_attr_interpulate(pack, pp.ca_en);

    /* gamma and ca lut */
    for (td_u32 i = 0; i < 33; i++) { /* 33 is index */
        scene_set_nrx_attr_interpulate(pack, pp.gamma_lut[i]);
        scene_set_nrx_attr_interpulate(pack, pp.ca_lut[i]);
    }
}

static td_void scene_set_3dnr_sfy1_5(const _3dnr_nrx_pack *pack)
{
    /* sf_y */
    for (td_u32 i = 0; i < 3; i++) { /* 3 sfy index */
        /* sfs1 */
        scene_set_nrx_attr_interpulate(pack, sfy[i].sfs1);
        scene_set_nrx_attr_interpulate(pack, sfy[i].sbr1);

        /* sfs2 */
        scene_set_nrx_attr_interpulate(pack, sfy[i].sfs2);
        scene_set_nrx_attr_interpulate(pack, sfy[i].sft2);
        scene_set_nrx_attr_interpulate(pack, sfy[i].sbr2);

        /* sfs4 */
        scene_set_nrx_attr_interpulate(pack, sfy[i].sfs4);
        scene_set_nrx_attr_interpulate(pack, sfy[i].sft4);
        scene_set_nrx_attr_interpulate(pack, sfy[i].sbr4);
    }

    for (td_u32 i = 1; i < 3; i++) { /* 3 sfy index */
        /* sfs3 and sfk4 */
        scene_set_nrx_attr_interpulate(pack, sfy[i].strf3);
        scene_set_nrx_attr_interpulate(pack, sfy[i].strb3);
        scene_set_nrx_attr_interpulate(pack, sfy[i].strf4);
        scene_set_nrx_attr_interpulate(pack, sfy[i].strb4);
    }
    scene_set_nrx_attr_interpulate(pack, sfy[1].sfs5); /* 1 index */
}

td_void scene_set_3dnr_sfy_lut(const _3dnr_nrx_pack *pack)
{
    /* N2 and N3 sfy_lut */
    for (td_u32 i = 0; i < 17; i++) { /* 17 is index */
        scene_set_nrx_attr_interpulate(pack, luty[0].sf_var_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf_var_b[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf_dir_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf_dir_b[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf_bri_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf_bri_b[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf_cor[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf_mot[i]);

        scene_set_nrx_attr_interpulate(pack, luty[0].sf5_var_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf5_var_b[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf5_dir_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf5_dir_b[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf5_bri_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf5_bri_b[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf5_sad_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf5_cor[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf5_mot[i]);

        scene_set_nrx_attr_interpulate(pack, luty[1].sf_var_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[1].sf_var_b[i]);
        scene_set_nrx_attr_interpulate(pack, luty[1].sf_dir_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[1].sf_dir_b[i]);
        scene_set_nrx_attr_interpulate(pack, luty[1].sf_bri_f[i]);
        scene_set_nrx_attr_interpulate(pack, luty[1].sf_bri_b[i]);
        scene_set_nrx_attr_interpulate(pack, luty[1].sf_cor[i]);
        scene_set_nrx_attr_interpulate(pack, luty[1].sf_mot[i]);

        scene_set_nrx_attr_interpulate(pack, luty[0].var_by_bri[i]);
        scene_set_nrx_attr_interpulate(pack, luty[0].sf_bld[i]);
        scene_set_nrx_attr_interpulate(pack, luty[1].var_by_bri[i]);
        scene_set_nrx_attr_interpulate(pack, luty[1].sf_bld[i]);
    }
}

static td_void scene_set_3dnr_sfy6_8(const _3dnr_nrx_pack *pack)
{
    ot_scene_3dnr *nrx_attr = pack->nrx_attr;
    const ot_scene_static_3dnr *nrx_param = pack->nrx_param;
    td_u32 iso_level = pack->iso_level;

    for (td_u32 i = 0; i < 3; i++) { /* 3 sfy index */
        nrx_attr->sfy[i].sfn6_0 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn6_0;
        nrx_attr->sfy[i].sfn6_1 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn6_1;

        nrx_attr->sfy[i].sfn0_0 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn0_0;
        nrx_attr->sfy[i].sfn1_0 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn1_0;
        nrx_attr->sfy[i].sfn2_0 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn2_0;
        nrx_attr->sfy[i].sfn3_0 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn3_0;

        nrx_attr->sfy[i].sfn0_1 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn0_1;
        nrx_attr->sfy[i].sfn1_1 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn1_1;
        nrx_attr->sfy[i].sfn2_1 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn2_1;
        nrx_attr->sfy[i].sfn3_1 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn3_1;

        scene_set_nrx_attr_interpulate(pack, sfy[i].sth1_0);
        scene_set_nrx_attr_interpulate(pack, sfy[i].sth2_0);
        scene_set_nrx_attr_interpulate(pack, sfy[i].sth3_0);

        scene_set_nrx_attr_interpulate(pack, sfy[i].sth1_1);
        scene_set_nrx_attr_interpulate(pack, sfy[i].sth2_1);
        scene_set_nrx_attr_interpulate(pack, sfy[i].sth3_1);
    }
    nrx_attr->sfy[0].bld6 = nrx_param->threednr_value[iso_level - 1].sfy[0].bld6;

    for (td_u32 i = 1; i < 3; i++) { /* 3 sfy index */
        nrx_attr->sfy[i].sfn7_0 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn7_0;
        nrx_attr->sfy[i].sfn7_1 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn7_1;
        nrx_attr->sfy[i].sfn8_0 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn8_0;
        nrx_attr->sfy[i].sfn8_1 = nrx_param->threednr_value[iso_level - 1].sfy[i].sfn8_1;
    }
}

td_void scene_set_3dnr_nrx_sfy(const _3dnr_nrx_pack *pack)
{
    scene_set_3dnr_sfy1_5(pack);
    scene_set_3dnr_sfy6_8(pack);
    scene_set_3dnr_sfy_lut(pack);
}

td_void scene_set_3dnr_nrx_tfy(const _3dnr_nrx_pack *pack)
{
    ot_scene_3dnr *nrx_attr = pack->nrx_attr;

    /* ref */
    scene_set_nrx_attr_interpulate(pack, tfy[1].ref_en);
    scene_set_nrx_attr_interpulate(pack, tfy[0].tfs_mode);

    /* tf_y */
    for (td_u32 i = 0; i < scene_array_size(nrx_attr->tfy); i++) {
        /* tss */
        scene_set_nrx_attr_interpulate(pack, tfy[i].tss0);
        scene_set_nrx_attr_interpulate(pack, tfy[i].tss1);
        scene_set_nrx_attr_interpulate(pack, tfy[i].tss2);

        /* tfs */
        scene_set_nrx_attr_interpulate(pack, tfy[i].tfs0);
        scene_set_nrx_attr_interpulate(pack, tfy[i].tfs1);
        scene_set_nrx_attr_interpulate(pack, tfy[i].tfs2);
    }

    /* tfr and tfs0_mot */
    for (td_u32 i = 0; i < scene_array_size(nrx_attr->tfy); i++) {
        for (td_u32 j = 0; j < 6; j++) { /* idx 6 */
            scene_set_nrx_attr_interpulate(pack, tfy[i].tfr0[j]);
            scene_set_nrx_attr_interpulate(pack, tfy[i].tfr1[j]);
        }
        for (td_u32 j = 0; j < 17; j++) { /* idx 17 */
            scene_set_nrx_attr_interpulate(pack, tfy[i].tfs0_mot[j]);
            scene_set_nrx_attr_interpulate(pack, tfy[i].tfs1_mot[j]);
        }
    }
}

td_void scene_set_3dnr_nrx_mdy(const _3dnr_nrx_pack *pack)
{
    /* md_y */
    for (td_u32 i = 0; i < scene_array_size(pack->nrx_attr->mdy); i++) {
        /* math */
        scene_set_nrx_attr_interpulate(pack, mdy[i].math0);
        scene_set_nrx_attr_interpulate(pack, mdy[i].math1);

        /* mate */
        scene_set_nrx_attr_interpulate(pack, mdy[i].mate0);
        scene_set_nrx_attr_interpulate(pack, mdy[i].mate1);

        /* mabw */
        scene_set_nrx_attr_interpulate(pack, mdy[i].mabw0);
        scene_set_nrx_attr_interpulate(pack, mdy[i].mabw1);
    }
    scene_set_nrx_attr_interpulate(pack, mdy0.tfs);
    scene_set_nrx_attr_interpulate(pack, mdy0.math);
    scene_set_nrx_attr_interpulate(pack, mdy0.mathd);
    scene_set_nrx_attr_interpulate(pack, mdy0.mabw);
    scene_set_nrx_attr_interpulate(pack, mdy0.tdz);

    for (td_u32 i = 0; i < 17; i++) { /* 17 is index */
        scene_set_nrx_attr_interpulate(pack, mdy0.tfs_mot[i]);
    }
}

td_void scene_set_3dnr_nrx_nrc0(const _3dnr_nrx_pack *pack)
{
    /* nrc0_en */
    scene_set_nrx_attr_interpulate(pack, nrc0_mode);
    /* tfs */
    scene_set_nrx_attr_interpulate(pack, nrc0.tfs);
    /* sfc */
    scene_set_nrx_attr_interpulate(pack, nrc0.sfc);
    /* tfc */
    scene_set_nrx_attr_interpulate(pack, nrc0.tfc);
    /* trc */
    scene_set_nrx_attr_interpulate(pack, nrc0.trc);

    for (td_u32 i = 0; i < 17; i++) { /* 17 is index */
        scene_set_nrx_attr_interpulate(pack, nrc0.tfs_mot[i]);
    }
}

td_void scene_set_3dnr_nrx_iey(const _3dnr_nrx_pack *pack)
{
    ot_scene_3dnr *nrx_attr = pack->nrx_attr;
    const ot_scene_static_3dnr *nrx_param = pack->nrx_param;
    td_u32 iso_level = pack->iso_level;

    scene_set_nrx_attr_interpulate(pack, iey.coarse_g1);
    scene_set_nrx_attr_interpulate(pack, iey.fine_g1);
    scene_set_nrx_attr_interpulate(pack, iey.coarse_g2);
    scene_set_nrx_attr_interpulate(pack, iey.fine_g2);
    scene_set_nrx_attr_interpulate(pack, iey.coarse_g3);
    scene_set_nrx_attr_interpulate(pack, iey.fine_g3);
    scene_set_nrx_attr_interpulate(pack, iey.coarse_g4);
    scene_set_nrx_attr_interpulate(pack, iey.fine_g4);
    scene_set_nrx_attr_interpulate(pack, iey.o_sht_f);
    scene_set_nrx_attr_interpulate(pack, iey.u_sht_f);
    scene_set_nrx_attr_interpulate(pack, iey.o_sht_b);
    scene_set_nrx_attr_interpulate(pack, iey.u_sht_b);
    scene_set_nrx_attr_interpulate(pack, iey.sth1_0);
    scene_set_nrx_attr_interpulate(pack, iey.sth2_0);
    scene_set_nrx_attr_interpulate(pack, iey.sth3_0);
    scene_set_nrx_attr_interpulate(pack, iey.sth1_1);
    scene_set_nrx_attr_interpulate(pack, iey.sth2_1);
    scene_set_nrx_attr_interpulate(pack, iey.sth3_1);

    nrx_attr->iey.sfn7_0 = nrx_param->threednr_value[iso_level - 1].iey.sfn7_0;
    nrx_attr->iey.sfn7_1 = nrx_param->threednr_value[iso_level - 1].iey.sfn7_1;
    nrx_attr->iey.sfn8_0 = nrx_param->threednr_value[iso_level - 1].iey.sfn8_0;
    nrx_attr->iey.sfn8_1 = nrx_param->threednr_value[iso_level - 1].iey.sfn8_1;

    nrx_attr->iey.sfn0_0 = nrx_param->threednr_value[iso_level - 1].iey.sfn0_0;
    nrx_attr->iey.sfn1_0 = nrx_param->threednr_value[iso_level - 1].iey.sfn1_0;
    nrx_attr->iey.sfn2_0 = nrx_param->threednr_value[iso_level - 1].iey.sfn2_0;
    nrx_attr->iey.sfn3_0 = nrx_param->threednr_value[iso_level - 1].iey.sfn3_0;

    nrx_attr->iey.sfn0_1 = nrx_param->threednr_value[iso_level - 1].iey.sfn0_1;
    nrx_attr->iey.sfn1_1 = nrx_param->threednr_value[iso_level - 1].iey.sfn1_1;
    nrx_attr->iey.sfn2_1 = nrx_param->threednr_value[iso_level - 1].iey.sfn2_1;
    nrx_attr->iey.sfn3_1 = nrx_param->threednr_value[iso_level - 1].iey.sfn3_1;

    for (td_u32 i = 0; i < 17; i++) { /* 17 is index */
        scene_set_nrx_attr_interpulate(pack, iey.sf_bld[i]);
        scene_set_nrx_attr_interpulate(pack, iey.shp_ctl_mean[i]);
        scene_set_nrx_attr_interpulate(pack, iey.shp_ctl_dir[i]);
    }
}

td_void scene_set_3dnr_nrx_nrc1(const _3dnr_nrx_pack *pack)
{
    /* nrc1_en */
    scene_set_nrx_attr_interpulate(pack, nrc_en);

    scene_set_nrx_attr_interpulate(pack, nrc1.pre_sfs);
    scene_set_nrx_attr_interpulate(pack, nrc1.sfs1);
    scene_set_nrx_attr_interpulate(pack, nrc1.sfs2_mode);
    scene_set_nrx_attr_interpulate(pack, nrc1.sfs2_coarse_f);
    scene_set_nrx_attr_interpulate(pack, nrc1.sfs2_coarse);
    scene_set_nrx_attr_interpulate(pack, nrc1.sfs2_fine_f);
    scene_set_nrx_attr_interpulate(pack, nrc1.sfs2_fine_b);

    for (td_u32 i = 0; i < 17; i++) { /* 17 is index */
        scene_set_nrx_attr_interpulate(pack, nrc1.sfs1_mot[i]);
    }
    for (td_u32 i = 0; i < 16; i++) { /* 16 is index */
        scene_set_nrx_attr_interpulate(pack, nrc1.sfs2_mot[i]);
    }
    for (td_u32 i = 0; i < 20; i++) { /* 20 is index */
        scene_set_nrx_attr_interpulate(pack, nrc1.sfs2_sat[i]);
    }
}

static td_void scene_set_qp_h265_avbr_param(ot_venc_rc_param *rc_param, td_u32 max_bitrate)
{
    if (max_bitrate <= 768) {                                   /* low than 768 */
        rc_param->h265_avbr_param.min_i_qp = 30;                /* min_i_qp is 30 */
        rc_param->h265_avbr_param.min_qp = 32;                  /* min_qp is 32 */
        rc_param->h265_avbr_param.max_i_qp = 48;                /* max_i_qp is 48 */
        rc_param->h265_avbr_param.max_qp = 48;                  /* max_qp is 48 */
    } else if ((max_bitrate > 768) && (max_bitrate <= 1536)) {  /* 768 to 1536 */
        rc_param->h265_avbr_param.min_i_qp = 29;                /* min_i_qp is 29 */
        rc_param->h265_avbr_param.min_qp = 31;                  /* min_qp is 31 */
        rc_param->h265_avbr_param.max_i_qp = 44;                /* x is 44 */
        rc_param->h265_avbr_param.max_qp = 44;                  /* max_qp is 44 */
    } else if ((max_bitrate > 1536) && (max_bitrate <= 2560)) { /* 1536 to 2560 */
        rc_param->h265_avbr_param.min_i_qp = 26;                /* min_i_qp is 26 */
        rc_param->h265_avbr_param.min_qp = 28;                  /* min_qp is 28 */
        rc_param->h265_avbr_param.max_i_qp = 42;                /* max_i_qp is 42 */
        rc_param->h265_avbr_param.max_qp = 42;                  /* max_qp is 42 */
    } else if ((max_bitrate > 2560) && (max_bitrate <= 3584)) { /* 2560 to 3584 */
        rc_param->h265_avbr_param.min_i_qp = 25;                /* min_i_qp is 25 */
        rc_param->h265_avbr_param.min_qp = 27;                  /* min_qp is 27 */
        rc_param->h265_avbr_param.max_i_qp = 42;                /* max_i_qp is 42 */
        rc_param->h265_avbr_param.max_qp = 42;                  /* max_qp is 42 */
    } else if ((max_bitrate > 3584) && (max_bitrate <= 4096)) { /* 3584 to 4096 */
        rc_param->h265_avbr_param.min_i_qp = 24;                /* min_i_qp is 24 */
        rc_param->h265_avbr_param.min_qp = 26;                  /* min_qp is 26 */
        rc_param->h265_avbr_param.max_i_qp = 42;                /* max_i_qp is 42 */
        rc_param->h265_avbr_param.max_qp = 42;                  /* max_qp is 42 */
    } else if (max_bitrate > 4096) {                            /* bigger than 4096 */
        rc_param->h265_avbr_param.min_i_qp = 23;                /* min_i_qp is 23 */
        rc_param->h265_avbr_param.min_qp = 25;                  /* min_qp is 25 */
        rc_param->h265_avbr_param.max_i_qp = 42;                /* max_i_qp is 42 */
        rc_param->h265_avbr_param.max_qp = 42;                  /* max_qp is 42 */
    }
    rc_param->h265_avbr_param.min_i_proportion = 1;
    rc_param->h265_avbr_param.max_i_proportion = 100; /* max_i_proportion is 100 */
    rc_param->h265_avbr_param.max_reencode_times = 2; /* max_reencode_times is 2 */

    rc_param->h265_avbr_param.min_still_percent = 25; /* min_still_percent is 25 */
    rc_param->h265_avbr_param.max_still_qp = 35;      /* max_still_qp is 35 */
    rc_param->h265_avbr_param.min_still_psnr = 0;
}

static td_void scene_set_qp_h264_avbr_param(ot_venc_rc_param *rc_param, td_u32 max_bitrate)
{
    if (max_bitrate <= 768) {                                   /* low than 768 */
        rc_param->h264_avbr_param.min_i_qp = 30;                /* min_i_qp is 30 */
        rc_param->h264_avbr_param.min_qp = 32;                  /* min_qp is 32 */
        rc_param->h264_avbr_param.max_i_qp = 48;                /* max_i_qp is 48 */
        rc_param->h264_avbr_param.max_qp = 48;                  /* max_qp is 48 */
    } else if ((max_bitrate > 768) && (max_bitrate <= 1536)) {  /* 768 to 1536 */
        rc_param->h264_avbr_param.min_i_qp = 29;                /* min_i_qp is 29 */
        rc_param->h264_avbr_param.min_qp = 31;                  /* min_qp is 31 */
        rc_param->h264_avbr_param.max_i_qp = 44;                /* max_i_qp is 44 */
        rc_param->h264_avbr_param.max_qp = 44;                  /* max_qp is 44 */
    } else if ((max_bitrate > 1536) && (max_bitrate <= 3072)) { /* 1536 to 3072 */
        rc_param->h264_avbr_param.min_i_qp = 27;                /* min_i_qp is 27 */
        rc_param->h264_avbr_param.min_qp = 29;                  /* min_qp is 29 */
        rc_param->h264_avbr_param.max_i_qp = 42;                /* max_i_qp is 42 */
        rc_param->h264_avbr_param.max_qp = 42;                  /* max_qp is 42 */
    } else if ((max_bitrate > 3072) && (max_bitrate <= 5120)) { /* 3072 to 5120 */
        rc_param->h264_avbr_param.min_i_qp = 26;                /* min_i_qp is 26 */
        rc_param->h264_avbr_param.min_qp = 28;                  /* min_qp is 28 */
        rc_param->h264_avbr_param.max_i_qp = 42;                /* max_i_qp is 42 */
        rc_param->h264_avbr_param.max_qp = 42;                  /* max_qp is 42 */
    } else if ((max_bitrate > 5120) && (max_bitrate <= 7168)) { /* 5120 to 7168 */
        rc_param->h264_avbr_param.min_i_qp = 25;                /* min_i_qp is 25 */
        rc_param->h264_avbr_param.min_qp = 27;                  /* min_qp is 27 */
        rc_param->h264_avbr_param.max_i_qp = 42;                /* max_i_qp is 42 */
        rc_param->h264_avbr_param.max_qp = 42;                  /* max_qp is 42 */
    } else if (max_bitrate > 7168) {                            /* bigger than 7168 */
        rc_param->h264_avbr_param.min_i_qp = 23;                /* min_i_qp is 23 */
        rc_param->h264_avbr_param.min_qp = 25;                  /* min_qp is 25 */
        rc_param->h264_avbr_param.max_i_qp = 42;                /* max_i_qp is 42 */
        rc_param->h264_avbr_param.max_qp = 42;                  /* max_qp is 42 */
    }
    rc_param->h264_avbr_param.min_i_proportion = 1;
    rc_param->h264_avbr_param.max_i_proportion = 100; /* max_qp is 100 */
    rc_param->h264_avbr_param.max_reencode_times = 2; /* max_qp is 2 */

    rc_param->h264_avbr_param.min_still_percent = 25; /* max_qp is 25 */
    rc_param->h264_avbr_param.max_still_qp = 35;      /* max_qp is 35 */
    rc_param->h264_avbr_param.min_still_psnr = 0;
}

td_s32 scene_set_qp(td_u32 pic_width, td_u32 pic_height, td_u32 max_bitrate, ot_payload_type type,
    ot_venc_rc_param *rc_param)
{
    ot_scenecomm_check_pointer_return(rc_param, TD_FAILURE);
    td_u32 align_pic_height;
    td_u32 align_pic_width;
    td_u32 align_pic_height_1080p;
    td_u32 align_pic_width_1080p;
    td_u32 bitrate;

    align_pic_height = ((pic_height + MB_ALIGN_H264 - 1) / MB_ALIGN_H264);
    align_pic_width = ((pic_width + MB_ALIGN_H264 - 1) / MB_ALIGN_H264);
    align_pic_height_1080p = ((PIC_HEIGHT_1080P + MB_ALIGN_H264 - 1) / MB_ALIGN_H264);
    align_pic_width_1080p = ((PIC_WIDTH_1080P + MB_ALIGN_H264 - 1) / MB_ALIGN_H264);
    bitrate = (align_pic_height_1080p * align_pic_width_1080p * max_bitrate) / (align_pic_height * align_pic_width);

    if (type == OT_PT_H265) {
        scene_set_qp_h265_avbr_param(rc_param, bitrate);
    }
    if (type == OT_PT_H264) {
        scene_set_qp_h264_avbr_param(rc_param, bitrate);
    }
    return TD_SUCCESS;
}

td_s32 get_iso_mean_qp_chn_attr(ot_vi_pipe vi_pipe, td_u32 *iso, td_u32 *mean_qp, ot_venc_chn_attr *venc_chn_attr)
{
    td_s32 ret;
    ot_isp_exp_info isp_exp_info;
    ot_venc_chn_status status;

    ret = ss_mpi_venc_get_chn_attr(vi_pipe, venc_chn_attr);
    check_scene_ret(ret);

    ret = ss_mpi_isp_query_exposure_info(vi_pipe, &isp_exp_info);
    check_scene_ret(ret);

    *iso = isp_exp_info.iso;

    ret = ss_mpi_venc_query_status(vi_pipe, &status);
    check_scene_ret(ret);

    *mean_qp = status.stream_info.mean_qp;

    return TD_SUCCESS;
}


td_void calculate_manual_percent_index(td_u8 pipe_param_index, td_u32 sum_iso, td_u32 *out_index)
{
    td_u32 i;
    for (i = 0; i < g_scene_pipe_param[pipe_param_index].dynamic_venc_bitrate.iso_thresh_cnt &&
         i < OT_SCENE_VENC_EXPOSURE_MAX_COUNT; i++) {
        if (sum_iso > g_scene_pipe_param[pipe_param_index].dynamic_venc_bitrate.iso_thresh_ltoh[i]) {
            (*out_index)++;
        }
    }
}

td_void set_initial_percent(ot_payload_type type, td_u32 index, ot_venc_rc_param *rc_param,
    const ot_scene_pipe_param *param, td_s32 *percent)
{
    if (type == OT_PT_H264) {
        rc_param->h264_avbr_param.chg_pos = param->dynamic_venc_bitrate.manual_percent[index - 1];
        *percent = rc_param->h264_avbr_param.chg_pos;
    }
    if (type == OT_PT_H265) {
        rc_param->h265_avbr_param.chg_pos = param->dynamic_venc_bitrate.manual_percent[index - 1];
        *percent = rc_param->h265_avbr_param.chg_pos;
    }
}

td_void set_min_qp_delta_when_iso_larger(ot_payload_type type, td_u32 sum_mean_qp, td_s32 percent,
    ot_venc_rc_param *rc_param)
{
    td_s32 local_percent = percent;
    if (sum_mean_qp <= 33 && sum_mean_qp >= 31) {        /* 31 to 33 */
        local_percent = 19 * percent / 20;               /* 19 multiplier, 20 is divider */
    } else if (sum_mean_qp <= 30 && sum_mean_qp >= 29) { /* 29 to 30 */
        local_percent = 9 * percent / 10;                /* 9 multiplier, 10 is divider */
    } else if (sum_mean_qp < 29) {                       /* low than 29 */
        local_percent = 8 * percent / 10;                /* 8 multiplier, 10 is divider */
    }

    if (type == OT_PT_H264) {
        rc_param->h264_avbr_param.chg_pos = local_percent;
        rc_param->h264_avbr_param.min_qp_delta = 2; /* min_qp_delta is 2 */
    }
    if (type == OT_PT_H265) {
        rc_param->h265_avbr_param.chg_pos = local_percent;
        rc_param->h265_avbr_param.min_qp_delta = 2; /* min_qp_delta is 2 */
    }
}

td_void set_min_qp_delta_when_iso_less(ot_payload_type type, td_u32 sum_mean_qp, ot_venc_rc_param *rc_param,
    const ot_venc_chn_attr *venc_chn_attr)
{
    if (venc_chn_attr->gop_attr.gop_mode == OT_VENC_GOP_MODE_SMART_P) {
        if (type == OT_PT_H264) {
            rc_param->h264_avbr_param.min_qp_delta = 3; /* min_qp_delta is 3 */
        }
        if (type == OT_PT_H265) {
            rc_param->h265_avbr_param.min_qp_delta = 3; /* min_qp_delta is 3 */
        }
    } else {
        if (type == OT_PT_H264) {
            rc_param->h264_avbr_param.min_qp_delta = 3; /* min_qp_delta is 3 */
        }
        if (type == OT_PT_H265) {
            rc_param->h265_avbr_param.min_qp_delta = 3; /* min_qp_delta is 3 */
        }
    }
}

#ifdef __cplusplus
}
#endif
