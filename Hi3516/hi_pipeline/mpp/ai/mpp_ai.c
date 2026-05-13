/**
 * @FilePath     : mpp_ai.c
 * @Author       : zhouzirui
 * @Date         : 2025-03-28 14:24:42
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-12 10:10:21
 * @Description  : 海思ai模块封装
 */

#include "mpp_ai.h"
#include "mpi_common.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define ACODEC_FILE "/dev/acodec"

/**
 * @brief       : 转换ot_audio_sample_rate采样率类型至ot_acodec_fs
 * @author      : zhouzirui
 * @param        {ot_audio_sample_rate} sample_rate：转换前的采样率
 * @param        {ot_acodec_fs} *i2s_fs：转换后的采样率
 * @return       {*}成功返回0,失败返回-1
 */
static td_s32 mppInnerCodec_get_i2sFs(ot_audio_sample_rate sample_rate, ot_acodec_fs *i2s_fs)
{
    ot_acodec_fs i2s_fs_sel;

    switch (sample_rate)
    {
    case OT_AUDIO_SAMPLE_RATE_8000:
        i2s_fs_sel = OT_ACODEC_FS_8000;
        break;
    case OT_AUDIO_SAMPLE_RATE_16000:
        i2s_fs_sel = OT_ACODEC_FS_16000;
        break;
    case OT_AUDIO_SAMPLE_RATE_32000:
        i2s_fs_sel = OT_ACODEC_FS_32000;
        break;
    case OT_AUDIO_SAMPLE_RATE_11025:
        i2s_fs_sel = OT_ACODEC_FS_11025;
        break;
    case OT_AUDIO_SAMPLE_RATE_22050:
        i2s_fs_sel = OT_ACODEC_FS_22050;
        break;
    case OT_AUDIO_SAMPLE_RATE_44100:
        i2s_fs_sel = OT_ACODEC_FS_44100;
        break;
    case OT_AUDIO_SAMPLE_RATE_12000:
        i2s_fs_sel = OT_ACODEC_FS_12000;
        break;
    case OT_AUDIO_SAMPLE_RATE_24000:
        i2s_fs_sel = OT_ACODEC_FS_24000;
        break;
    case OT_AUDIO_SAMPLE_RATE_48000:
        i2s_fs_sel = OT_ACODEC_FS_48000;
        break;
    case OT_AUDIO_SAMPLE_RATE_64000:
        i2s_fs_sel = OT_ACODEC_FS_64000;
        break;
    case OT_AUDIO_SAMPLE_RATE_96000:
        i2s_fs_sel = OT_ACODEC_FS_96000;
        break;
    default:
        mpi_ai_log("not support sample_rate:%d", sample_rate);
        return TD_FAILURE;
    }

    *i2s_fs = i2s_fs_sel;
    return TD_SUCCESS;
}

/**
 * @brief       : 配置内部音频编解码器
 * @author      : zhouzirui
 * @param        {ot_audio_sample_rate} sample_rate：采样率
 * @return       {*}成功返回0,失败返回-1
 */
td_s32 mppInnerCodec_cfg_audio(ot_audio_sample_rate sample_rate)
{
    td_s32 ret;
    td_s32 fd_acodec = -1;
    ot_acodec_fs i2s_fs_sel;
    ot_acodec_mixer input_mode;

    fd_acodec = open(ACODEC_FILE, O_RDWR);
    if (fd_acodec < 0)
    {
        mpi_ai_log("can't open audio codec,%s", ACODEC_FILE);
        return TD_FAILURE;
    }

    ret = ioctl(fd_acodec, OT_ACODEC_SOFT_RESET_CTRL);
    if (ret != TD_SUCCESS)
    {
        mpi_ai_log("reset audio codec error");
        goto cfg_fail;
    }

    ret = mppInnerCodec_get_i2sFs(sample_rate, &i2s_fs_sel);
    if (ret != TD_SUCCESS)
    {
        goto cfg_fail;
    }

    ret = ioctl(fd_acodec, OT_ACODEC_SET_I2S1_FS, &i2s_fs_sel);
    if (ret != TD_SUCCESS)
    {
        mpi_ai_log("set acodec sample rate failed");
        goto cfg_fail;
    }

    /* refer to hardware, demo board is pseudo-differential (IN_D), socket board is single-ended (IN1) */
    input_mode = OT_ACODEC_MIXER_IN_D;
    ret = ioctl(fd_acodec, OT_ACODEC_SET_MIXER_MIC, &input_mode);
    if (ret != TD_SUCCESS)
    {
        mpi_ai_log("select acodec input_mode failed");
        goto cfg_fail;
    }

    /*
     * 输入音量范围为[-78, 80]。模拟增益和数字增益均会被调节。
     * 数值越大表示音量越高。
     * 例如，数值80表示最大音量80分贝，
     * 数值-78表示最小音量（静音状态）。
     * 音量调节会同时作用于左右音频声道。
     * 推荐音量范围为[20, 50]。
     * 在此范围内仅调节模拟增益，因此噪声最低，
     * 且能保证语音质量。
     */
    int acodec_input_vol;

    acodec_input_vol = 50; /* 30dB */
    ret = ioctl(fd_acodec, OT_ACODEC_SET_INPUT_VOLUME, &acodec_input_vol);
    if (ret != TD_SUCCESS)
    {
        mpi_ai_log("set acodec micin volume failed");
        goto cfg_fail;
    }

    mpi_ai_log("set inner audio codec ok: sample_rate = %d.", sample_rate);

cfg_fail:
    close(fd_acodec);
    return ret;
}

/**
 * @brief   : 声音质量增强（Record）配置信息结构体属性填充
 * @param    {HiAi_S} *pHandle
 * @param    {ot_ai_record_vqe_cfg} *pstAiRecordVqeCfg 声音质量增强（Record）配置信息结构体
 * @param    {td_bool} bEnableNr 是否使能录音噪声消除
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppAi_ot_ai_record_vqe_cfg_fill(HiAi_S *pHandle, ot_ai_record_vqe_cfg *pstAiRecordVqeCfg, td_bool bEnableNr)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /* 判断是否使能降噪 */
    if (bEnableNr == TD_TRUE)
    {
        pstAiRecordVqeCfg->open_mask = OT_AI_RECORDVQE_MASK_HPF | OT_AI_RECORDVQE_MASK_RNR | OT_AI_RECORDVQE_MASK_HDR |
                                     OT_AI_RECORDVQE_MASK_DRC | OT_AI_RECORDVQE_MASK_EQ | OT_AI_RECORDVQE_MASK_AGC;
    }
    else if (bEnableNr == TD_FALSE)
    {
        pstAiRecordVqeCfg->open_mask = OT_AI_RECORDVQE_MASK_HPF | OT_AI_RECORDVQE_MASK_HDR | OT_AI_RECORDVQE_MASK_DRC |
                                     OT_AI_RECORDVQE_MASK_EQ | OT_AI_RECORDVQE_MASK_AGC;
    }

    pstAiRecordVqeCfg->work_sample_rate = pHandle->stNeedParam.enSampleRate;
    pstAiRecordVqeCfg->frame_sample = pHandle->stNeedParam.u32PointNumPerFrame;
    pstAiRecordVqeCfg->work_state = OT_VQE_WORK_STATE_NOISY;
    pstAiRecordVqeCfg->in_chn_num = pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_MONO     ? 1
                                  : pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_STEREO ? 2
                                                                                                   : 1;
    pstAiRecordVqeCfg->out_chn_num = pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_MONO     ? 1
                                   : pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_STEREO ? 2
                                                                                                    : 1;
    pstAiRecordVqeCfg->record_type = OT_VQE_RECORD_NORMAL;
    /*场景模式 COMMON按照标准写死配置*/
    /*高通滤波*/
    pstAiRecordVqeCfg->hpf_cfg.usr_mode = TD_FALSE;
    pstAiRecordVqeCfg->hpf_cfg.hpf_freq = OT_AUDIO_HPF_FREQ_120;
    /*录音噪声消除*/
    pstAiRecordVqeCfg->rnr_cfg.usr_mode = TD_FALSE;
    pstAiRecordVqeCfg->rnr_cfg.nr_mode = 1;
    pstAiRecordVqeCfg->rnr_cfg.max_nr_level = 18;
    pstAiRecordVqeCfg->rnr_cfg.noise_threshold = -25;
    /*高动态范围*/
    pstAiRecordVqeCfg->hdr_cfg.usr_mode = TD_FALSE;
    /*动态压缩控制*/
    pstAiRecordVqeCfg->drc_cfg.usr_mode = TD_FALSE;
    pstAiRecordVqeCfg->drc_cfg.attack_time = 24;
    pstAiRecordVqeCfg->drc_cfg.release_time = 100;
    td_s16 old_level_db[OT_VQE_DRC_SEC_NUM] = { 0, -472, -792, -960, -1280 };
    memcpy(pstAiRecordVqeCfg->drc_cfg.old_level_db, old_level_db, sizeof(old_level_db));
    td_s16 new_level_db[OT_VQE_DRC_SEC_NUM] = { 0, -174, -528, -736, -1200 };
    memcpy(pstAiRecordVqeCfg->drc_cfg.new_level_db, new_level_db, sizeof(new_level_db));
    /*均衡器*/
    td_s8 gain_db[OT_VQE_EQ_BAND_NUM] = {
        -10, // 100Hz     - 强力衰减，抑制低频噪声（空调、风扇、振动）
        -6,  // 200Hz     - 衰减，防止“嗡嗡”声
        -4,  // 250Hz     - 轻微衰减，避免浑浊
        -2,  // 350Hz     - 微衰，保留部分能量
        0,   // 500Hz     - 平直，保留元音基础
        2,   // 800Hz     - 微升，增强中频可懂度
        4,   // 1.2kHz    - 提升，增强语音存在感
        6,   // 2.5kHz    - 重点提升！关键辅音（s, t, k）
        6,   // 4kHz      - 重点提升！擦音和清晰度
        2    // 8kHz      - 若采样率支持（≥16kHz），可轻微提升空气感；否则设为0
    };
    memcpy_s(pstAiRecordVqeCfg->eq_cfg.gain_db, sizeof(pstAiRecordVqeCfg->eq_cfg.gain_db), gain_db, sizeof(gain_db));
    /*自动增益*/
    pstAiRecordVqeCfg->agc_cfg.usr_mode = TD_TRUE;
    pstAiRecordVqeCfg->agc_cfg.target_level = -6;
    pstAiRecordVqeCfg->agc_cfg.noise_floor = -40;
    pstAiRecordVqeCfg->agc_cfg.max_gain = 15;
    pstAiRecordVqeCfg->agc_cfg.adjust_speed = 8;
    pstAiRecordVqeCfg->agc_cfg.improve_snr = 1;
    pstAiRecordVqeCfg->agc_cfg.use_hpf = 3;
    pstAiRecordVqeCfg->agc_cfg.output_mode = 0;
    pstAiRecordVqeCfg->agc_cfg.noise_suppress_switch = 0;

    return TD_SUCCESS;
}

/**
 * @brief   : 声音质量增强（Talk Vqe V2）配置信息结构体属性填充
 * @param    {HiAi_S} *pHandle
 * @param    {ot_ai_talk_vqe_v2_cfg} *pstVqeCfg 声音质量增强（Talk Vqe V2）配置信息结构体
 * @param    {td_bool} bEnableNr 是否使能降噪
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppAi_ot_ai_talk_vqe_v2_cfg_fill(HiAi_S *pHandle, ot_ai_talk_vqe_v2_cfg *pstVqeCfg, td_bool bEnableNr)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /* 判断是否使能降噪 */
    if (bEnableNr == TD_TRUE)
    {
        pstVqeCfg->open_mask = OT_AI_TALKVQEV2_MASK_PNR | OT_AI_TALKVQEV2_MASK_NR | OT_AI_TALKVQEV2_MASK_AGC
                               | OT_AI_TALKVQEV2_MASK_EQ | OT_AI_TALKVQEV2_MASK_FMP | OT_AI_TALKVQEV2_MASK_AEC
                               | OT_AI_TALKVQEV2_MASK_HS;
    }
    else if (bEnableNr == TD_FALSE)
    {
        pstVqeCfg->open_mask = OT_AI_TALKVQEV2_MASK_AGC | OT_AI_TALKVQEV2_MASK_EQ
                               | OT_AI_TALKVQEV2_MASK_FMP | OT_AI_TALKVQEV2_MASK_AEC | OT_AI_TALKVQEV2_MASK_HS;
    }

    pstVqeCfg->work_sample_rate = pHandle->stNeedParam.enSampleRate;
    pstVqeCfg->frame_sample = pHandle->stNeedParam.u32PointNumPerFrame;
    pstVqeCfg->work_state = OT_VQE_WORK_STATE_COMMON;
    pstVqeCfg->in_chn_num = pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_MONO     ? 1
                            : pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_STEREO ? 2
                                                                                             : 1;
    pstVqeCfg->out_chn_num = pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_MONO     ? 1
                             : pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_STEREO ? 2
                                                                                              : 1;
    /*场景模式 COMMON按照标准写死配置*/
    /* 二次降噪 */
    pstVqeCfg->pnr_cfg.usr_mode = TD_TRUE;
    pstVqeCfg->pnr_cfg.min_gain_limit = 5827;
    pstVqeCfg->pnr_cfg.snr_prior_limit = 1036;
    pstVqeCfg->pnr_cfg.ht_threshold = 10;
    pstVqeCfg->pnr_cfg.hs_threshold = 100;
    pstVqeCfg->pnr_cfg.alpha_ph = 90;
    pstVqeCfg->pnr_cfg.alpha_psd = 65;
    pstVqeCfg->pnr_cfg.prior_snr_fixed = 30;
    pstVqeCfg->pnr_cfg.cep_threshold = 16;
    pstVqeCfg->pnr_cfg.cep_amp = 120;
    pstVqeCfg->pnr_cfg.low_freq_protect = 1;
    pstVqeCfg->pnr_cfg.speech_protect_threshold = 75;
    pstVqeCfg->pnr_cfg.hem_enable = 0;
    pstVqeCfg->pnr_cfg.tcs_enable = 1;
    /* 降噪 */
    pstVqeCfg->nr_cfg.usr_mode = TD_TRUE;
    pstVqeCfg->nr_cfg.min_gain_limit = 5827;
    pstVqeCfg->nr_cfg.snr_prior_limit = 1036;
    pstVqeCfg->nr_cfg.ht_threshold = 130;
    pstVqeCfg->nr_cfg.hs_threshold = 100;
    pstVqeCfg->nr_cfg.prior_snr = 20;
    pstVqeCfg->nr_cfg.snr_smooth_factor = 6666;
    pstVqeCfg->nr_cfg.speech_prob_smooth_factor = 7900;
    pstVqeCfg->nr_cfg.noise_pwr_smooth_factor = 7594;
    pstVqeCfg->nr_cfg.low_freq_suppress_enable = 1;
    pstVqeCfg->nr_cfg.low_freq_gain_suppress = 2;
    pstVqeCfg->nr_cfg.env_mode = 1;
    pstVqeCfg->nr_cfg.cep_alpha = 80;
    pstVqeCfg->nr_cfg.cep_threshold = 10;
    pstVqeCfg->nr_cfg.cep_amp = 200;
    pstVqeCfg->nr_cfg.gain_sm_mode = 1;
    pstVqeCfg->nr_cfg.gain_sm_alpha1 = 30;
    pstVqeCfg->nr_cfg.gain_sm_alpha2 = 70;
    pstVqeCfg->nr_cfg.gain_sm_alpha3 = 30;
    /*自动增益*/
    pstVqeCfg->agc_cfg.usr_mode = TD_TRUE;
    pstVqeCfg->agc_cfg.target_level = -16;
    pstVqeCfg->agc_cfg.max_gain = 96;
    pstVqeCfg->agc_cfg.min_gain = -60;
    pstVqeCfg->agc_cfg.up_gradient_ratio = 9;
    pstVqeCfg->agc_cfg.down_gradient_ratio = 3;
    pstVqeCfg->agc_cfg.decay = -260;
    pstVqeCfg->agc_cfg.vad_threshold = 100;
    pstVqeCfg->agc_cfg.vad_ctrl = 1;
    /*均衡器*/
    pstVqeCfg->eq_cfg.usr_mode = TD_TRUE;
    td_s8 gain_db[OT_TALKVQEV2_EQ_BAND_NUM] = { -40, -40, -15, -15, -15, -13, -10, 0, 0, 0, 1, 1, 1, 1, 1,
                                                1,   1,   1,   1,   1,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0 };
    memcpy_s(pstVqeCfg->eq_cfg.gain_db, sizeof(pstVqeCfg->eq_cfg.gain_db), gain_db, sizeof(gain_db));
    /* 并行处理 */
    pstVqeCfg->fmp_cfg.usr_mode = TD_TRUE;
    pstVqeCfg->fmp_cfg.comfort_flag = 1;
    pstVqeCfg->fmp_cfg.comfort_intensity = 3;
    /* 自动回声抵消 */
    pstVqeCfg->aec_cfg.usr_mode = TD_TRUE;
    pstVqeCfg->aec_cfg.pure_delay = 0;
    pstVqeCfg->aec_cfg.switch_nlp = 1;

    pstVqeCfg->aec_cfg.band1 = 100;
    pstVqeCfg->aec_cfg.band2 = 1500;
    pstVqeCfg->aec_cfg.band3 = 3000;
    pstVqeCfg->aec_cfg.band4 = 4500;

    pstVqeCfg->aec_cfg.gain_lower_limit1 = 0;
    pstVqeCfg->aec_cfg.gain_lower_limit2 = 0;
    pstVqeCfg->aec_cfg.gain_lower_limit3 = 0;
    pstVqeCfg->aec_cfg.gain_lower_limit4 = 0;
    pstVqeCfg->aec_cfg.gain_lower_limit5 = 0;

    pstVqeCfg->aec_cfg.ols_on = 1;
    pstVqeCfg->aec_cfg.speaker_nl_on = 1;
    pstVqeCfg->aec_cfg.block_num = 6;
    
    pstVqeCfg->aec_cfg.echo_boost1 = 1;
    pstVqeCfg->aec_cfg.echo_boost2 = 4;
    pstVqeCfg->aec_cfg.echo_boost3 = 12;
    pstVqeCfg->aec_cfg.echo_boost4 = 4;
    pstVqeCfg->aec_cfg.echo_boost5 = 1;
    /* 降风噪 */
    // pstVqeCfg->wnr_cfg.usr_mode = TD_FALSE;
    /* 抗啸叫 */
    pstVqeCfg->hs_cfg.usr_mode = TD_TRUE;
    pstVqeCfg->hs_cfg.hold_time = 100;
    pstVqeCfg->hs_cfg.min_gain = 1;
    pstVqeCfg->hs_cfg.threshold = 20;
    pstVqeCfg->hs_cfg.smooth_time = 200;
    pstVqeCfg->hs_cfg.freq_move = 5;

    return TD_SUCCESS;
}

/**
 * @brief       : ai初始化
 * @author      : zhouzirui
 * @param        {HiAi_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAi_init(HiAi_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    
    /*ai设备号*/
    ot_audio_dev nAiDev = pHandle->stExParam.nDevId;
    /*ai通道号*/
    // ot_ai_chn nAiChn = pHandle->stNeedParam.nChn;

    ot_aio_attr stAioAttr = {
        .sample_rate = pHandle->stNeedParam.enSampleRate,
        .bit_width = pHandle->stNeedParam.enBitWidth,
        .work_mode = OT_AIO_MODE_I2S_MASTER,
        .snd_mode = pHandle->stNeedParam.enSoundMode,
        .expand_flag = 0,
        .frame_num = pHandle->stNeedParam.u32FrameNum,
        .point_num_per_frame = pHandle->stNeedParam.u32PointNumPerFrame,
        .chn_cnt = (pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_MONO)     ? (td_u32) 1
                   : (pHandle->stNeedParam.enSoundMode == OT_AUDIO_SOUND_MODE_STEREO) ? 2
                                                                                      : 1,
        .clk_share = 1,
        .i2s_type = OT_AIO_I2STYPE_INNERCODEC,
    };
    /*设置AI设备属性*/
    CHECK_API_RETURN(ss_mpi_ai_set_pub_attr(nAiDev,&stAioAttr));
    /*启用AI设备*/
    CHECK_API_RETURN(ss_mpi_ai_enable(nAiDev));

    /* 设置AI声道模式 */
    CHECK_API_RETURN(ss_mpi_ai_set_track_mode(nAiDev, pHandle->stExParam.enTrackMode));

    for (int nChn = 0; nChn < pHandle->stNeedParam.nChnNum; nChn++)
    {
        // note 获取raw原始数据时使用
        // ot_ai_chn_attr stChnAttr;
        // /* 获取AI通道属性 */
        // CHECK_API_RETURN(ss_mpi_ai_get_chn_attr(nAiDev, nChn, &stChnAttr));
        // stChnAttr.raw_frame_enable = TD_TRUE;
        // /* 设置AI通道属性 */
        // CHECK_API_RETURN(ss_mpi_ai_set_chn_attr(nAiDev, nChn, &stChnAttr));
        /*启用AI通道*/
        CHECK_API_RETURN(ss_mpi_ai_enable_chn(nAiDev, nChn));

        /*是否启用重采样功能*/
        if(pHandle->stNeedParam.nResampleEnable)
        {
            /*启用AI重采样*/
            CHECK_API_RETURN(ss_mpi_ai_enable_resample(nAiDev, nChn, pHandle->stNeedParam.enResampleRate));
        }
        /*是否启用vqe声音质量增强功能*/
        if (pHandle->stNeedParam.nVqeEnable == TD_TRUE)
        {
            /*判断使用vqe算法类型，设置值*/
            switch (pHandle->stNeedParam.enVqeType)
            {
            case AUDIO_VQE_TYPE_RECORD:
            {
                ot_ai_record_vqe_cfg stAiRecordVqeCfg;
                mppAi_ot_ai_record_vqe_cfg_fill(pHandle, &stAiRecordVqeCfg, pHandle->stExParam.bEnableNr);
                /*设置AI的声音质量增强功能（Record）相关属性*/
                CHECK_API_RETURN(ss_mpi_ai_set_record_vqe_attr(nAiDev, nChn, &stAiRecordVqeCfg));
                break;
            }
            case AUDIO_VQE_TYPE_TALK:
            case AUDIO_VQE_TYPE_TALKV2:
            {
                ot_ai_talk_vqe_v2_cfg stAiTalkVqeV2Cfg;
                mppAi_ot_ai_talk_vqe_v2_cfg_fill(pHandle, &stAiTalkVqeV2Cfg, pHandle->stExParam.bEnableNr);
                /*设置AI的声音质量增强功能（Talk V2）相关属性*/
                CHECK_API_RETURN(ss_mpi_ai_set_talk_vqe_v2_attr(nAiDev, nChn, pHandle->stNeedParam.nAoDev, pHandle->stNeedParam.nAoChn, &stAiTalkVqeV2Cfg));
                break;
            }
            default:
                return TD_FAILURE;
                break;
            }
            /*使能AI的声音质量增强功能*/
            CHECK_API_RETURN(ss_mpi_ai_enable_vqe(nAiDev, nChn));
        }
    }

    /* 配置内部音频编解码器 */
    CHECK_API_RETURN(mppInnerCodec_cfg_audio(pHandle->stNeedParam.enSampleRate));

    return TD_SUCCESS;
}

/**
 * @brief       : ai去初始化
 * @author      : zhouzirui
 * @param        {HiAi_S} *pHandle：句柄
 * @return       {*}成功返回0,失败返回-1
 */
static int mppAi_uninit(HiAi_S *pHandle)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }
    /*ai设备号*/
    ot_audio_dev nAiDev = pHandle->stExParam.nDevId;
    /*ai通道号*/
    // ot_ai_chn nAiChn = pHandle->stNeedParam.nChn;

    for (int nChn = 0; nChn < pHandle->stNeedParam.nChnNum; nChn++)
    {
        if(pHandle->stNeedParam.nResampleEnable)
        {
            CHECK_API_RETURN(ss_mpi_ai_disable_resample(nAiDev, nChn));
        }
        if (pHandle->stNeedParam.nVqeEnable == TD_TRUE)
        {
            /*禁用AI的声音质量增强功能*/
            CHECK_API_RETURN(ss_mpi_ai_disable_vqe(nAiDev,nChn));
        }
        /*禁用AI通道*/
        CHECK_API_RETURN(ss_mpi_ai_disable_chn(nAiDev, nChn));
    }
    CHECK_API_RETURN(ss_mpi_ai_disable(nAiDev));

    return TD_SUCCESS;
}

/** 
 * @brief   : 获取采集帧
 * @param    {HiAi_S} *pHandle：句柄
 * @param    {int} nChn：通道号
 * @param    {ot_audio_frame} *pFrame：帧数据信息指针
 * @param    {ot_aec_frame} *pAecFrame：回声抵消参考帧结构体指针
 * @param    {int} nTimeoutMs：等待时间
 * @return   {int}成功返回0,失败返回-1
 */
static int mppAi_getFrame(HiAi_S *pHandle, int nChn, ot_audio_frame *pFrame, ot_aec_frame *pAecFrame,int nTimeoutMs)
{
    if (NULL == pHandle || NULL == pFrame || nTimeoutMs < -1)
    {
        return TD_FAILURE;
    }

    if(pHandle->stNeedParam.enVqeType != AUDIO_VQE_TYPE_RECORD && pAecFrame == NULL)
    {
        return TD_FAILURE;
    }

    CHECK_API_RETURN(ss_mpi_ai_get_frame(pHandle->stExParam.nDevId, nChn, pFrame, pAecFrame, nTimeoutMs));

    return TD_SUCCESS;
}

/**
 * @brief   : 释放采集帧
 * @param    {HiAi_S} *pHandle：句柄
 * @param    {int} nChn：通道号
 * @param    {ot_audio_frame} *pFrame：帧数据信息指针
 * @param    {ot_aec_frame} *pAecFrame：回声抵消参考帧结构体指针
 * @return   {int}成功返回0,失败返回-1
 */
static int mppAi_releaseFrame(HiAi_S *pHandle, int nChn, ot_audio_frame *pFrame, ot_aec_frame *pAecFrame)
{
    if (NULL == pHandle || NULL == pFrame)
    {
        return TD_FAILURE;
    }

    if ((pHandle->stNeedParam.enVqeType == AUDIO_VQE_TYPE_RECORD
         || pHandle->stNeedParam.enVqeType != AUDIO_VQE_TYPE_TALKV2)
        && pAecFrame == NULL)
    {
        return TD_FAILURE;
    }

    CHECK_API_RETURN(ss_mpi_ai_release_frame(pHandle->stExParam.nDevId, nChn, pFrame, pAecFrame));

    return TD_SUCCESS;
}

/** 
 * @brief   : 获取采集音频原始帧
 * @param    {HiAi_S} *pHandle：句柄
 * @param    {int} nChn：通道号
 * @param    {ot_audio_frame} *pFrame：帧数据信息指针
 * @param    {int} nTimeoutMs：等待时间
 * @return   {int}成功返回0,失败返回-1
 */
static int mppAi_getRawFrame(HiAi_S *pHandle, int nChn, ot_audio_frame *pFrame, int nTimeoutMs)
{
    if (NULL == pHandle || NULL == pFrame || nTimeoutMs < -1)
    {
        return TD_FAILURE;
    }

    CHECK_API_RETURN(ss_mpi_ai_get_raw_frame(pHandle->stExParam.nDevId, nChn, pFrame, NULL, nTimeoutMs));

    return TD_SUCCESS;
}

/**
* @brief   : 释放采集音频原始帧
* @param    {HiAi_S} *pHandle：句柄
* @param    {int} nChn：通道号
* @param    {ot_audio_frame} *pFrame：帧数据信息指针
* @return   {int}成功返回0,失败返回-1
*/
static int mppAi_releaseRawFrame(HiAi_S *pHandle, int nChn, ot_audio_frame *pFrame)
{
    if (NULL == pHandle || NULL == pFrame)
    {
        return TD_FAILURE;
    }

    CHECK_API_RETURN(ss_mpi_ai_release_raw_frame(pHandle->stExParam.nDevId, nChn, pFrame, NULL));

    return TD_SUCCESS;
}

/**
 * @brief   : 是否使能vqe声音质量增强
 * @param    {HiAi_S} *pHandle 句柄
 * @param    {int} nChn 通道号
 * @param    {td_bool} nVqeEnable 是否使能
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppAi_whether_enable_vqe(HiAi_S *pHandle, int nChn, td_bool nVqeEnable)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /* ai设备号 */
    ot_audio_dev nAiDev = pHandle->stExParam.nDevId;

    if(nVqeEnable == TD_TRUE)
    {
        /* 使能AI的声音质量增强功能 */
        CHECK_API_RETURN(ss_mpi_ai_enable_vqe(nAiDev, nChn));
    }
    else if(nVqeEnable == TD_FALSE)
    {
        /* 不使能AI的声音质量增强功能 */
        CHECK_API_RETURN(ss_mpi_ai_disable_vqe(nAiDev,nChn));
    }

    pHandle->stNeedParam.nVqeEnable = nVqeEnable;

    return TD_SUCCESS;
}

/**
 * @brief   : 是否使能录音噪声消除
 * @param    {HiAi_S} *pHandle 句柄
 * @param    {int} nChn 通道号
 * @param    {td_bool} bEnableNr 是否使能
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppAi_whether_enable_vqe_rnr(HiAi_S *pHandle, int nChn, td_bool bEnableNr)
{
    if (NULL == pHandle || TD_FALSE == pHandle->stNeedParam.nVqeEnable)
    {
        return TD_FAILURE;
    }

    /* ai设备号 */
    ot_audio_dev nAiDev = pHandle->stExParam.nDevId;
    /* AEC ao 设备号 */
    ot_audio_dev nAoDev = pHandle->stNeedParam.nAoDev;
    /* AEC ao 通道号 */
    ot_ao_chn nAoChn = pHandle->stNeedParam.nAoChn;

    if (bEnableNr != pHandle->stExParam.bEnableNr)
    {
        /* 禁用AI的声音质量增强功能 */
        CHECK_API_RETURN(ss_mpi_ai_disable_vqe(nAiDev, nChn));
        if (pHandle->stNeedParam.enVqeType == AUDIO_VQE_TYPE_RECORD)
        {
            ot_ai_record_vqe_cfg stAiVqeCfg;
            /* 获取AI的声音质量增强功能（Record）相关属性 */
            CHECK_API_RETURN(ss_mpi_ai_get_record_vqe_attr(nAiDev, nChn, &stAiVqeCfg));
            mppAi_ot_ai_record_vqe_cfg_fill(pHandle, &stAiVqeCfg, bEnableNr);
            /* 设置AI的声音质量增强功能（Record）相关属性 */
            CHECK_API_RETURN(ss_mpi_ai_set_record_vqe_attr(nAiDev, nChn, &stAiVqeCfg));
        }
        else if (pHandle->stNeedParam.enVqeType == AUDIO_VQE_TYPE_TALKV2)
        {
            ot_ai_talk_vqe_v2_cfg stAiVqeCfg;
            /* 获取AI的声音质量增强功能（Record）相关属性 */
            CHECK_API_RETURN(ss_mpi_ai_get_talk_vqe_v2_attr(nAiDev, nChn, &stAiVqeCfg));
            mppAi_ot_ai_talk_vqe_v2_cfg_fill(pHandle, &stAiVqeCfg, bEnableNr);
            /* 设置AI的声音质量增强功能（Record）相关属性 */
            CHECK_API_RETURN(ss_mpi_ai_set_talk_vqe_v2_attr(nAiDev, nChn, nAoDev, nAoChn, &stAiVqeCfg));
        }
        /* 使能AI的声音质量增强功能 */
        CHECK_API_RETURN(ss_mpi_ai_enable_vqe(nAiDev, nChn));
    }

    pHandle->stExParam.bEnableNr = bEnableNr;

    return TD_SUCCESS;
}

/**
 * @brief   : 设置AI声道模式
 * @param    {HiAi_S} *pHandle 句柄
 * @param    {ot_audio_track_mode} enTrackMode 声道模式
 * @return   {int} 成功返回0,失败返回-1
 */
static int mppAi_set_track_mode(HiAi_S *pHandle, ot_audio_track_mode enTrackMode)
{
    if (NULL == pHandle)
    {
        return TD_FAILURE;
    }

    /* ai设备号 */
    ot_audio_dev nAiDev = pHandle->stExParam.nDevId;

    if (enTrackMode != pHandle->stExParam.enTrackMode)
    {
        /* 设置AI声道模式 */
        CHECK_API_RETURN(ss_mpi_ai_set_track_mode(nAiDev, enTrackMode));
    }

    pHandle->stExParam.enTrackMode = enTrackMode;

    return TD_SUCCESS;
}

HiAi_S *mppAi_alloc(HiAiNeedParam_S stNeedParam)
{
    HiAi_S *pHandle = (HiAi_S *)malloc(sizeof(HiAi_S));
    memset(pHandle, 0, sizeof(HiAi_S));

    //info /**********************必需参数***************************/
    pHandle->stNeedParam.nChn                   = stNeedParam.nChn;
    pHandle->stNeedParam.enBitWidth             = stNeedParam.enBitWidth;
    pHandle->stNeedParam.enSampleRate           = stNeedParam.enSampleRate;
    pHandle->stNeedParam.enSoundMode            = stNeedParam.enSoundMode;
    pHandle->stNeedParam.u32FrameNum            = stNeedParam.u32FrameNum;
    pHandle->stNeedParam.u32PointNumPerFrame    = stNeedParam.u32PointNumPerFrame;
    pHandle->stNeedParam.nChnNum                = stNeedParam.nChnNum;
    pHandle->stNeedParam.nResampleEnable        = stNeedParam.nResampleEnable;
    pHandle->stNeedParam.enResampleRate         = stNeedParam.enResampleRate;
    pHandle->stNeedParam.nVqeEnable             = stNeedParam.nVqeEnable;
    pHandle->stNeedParam.enVqeType              = stNeedParam.enVqeType;
    pHandle->stNeedParam.nAoDev                 = stNeedParam.nAoDev;
    pHandle->stNeedParam.nAoChn                 = stNeedParam.nAoChn;
    //info /**********************功能参数***************************/
    pHandle->stExParam.nDevId                   = 0;
    pHandle->stExParam.bEnableNr               = TD_TRUE;
    pHandle->stExParam.enTrackMode              = OT_AUDIO_TRACK_NORMAL;

    //info /**********************函数列表***************************/
    pHandle->mppAi_init                     = mppAi_init;
    pHandle->mppAi_uninit                   = mppAi_uninit;
    pHandle->mppAi_getFrame                 = mppAi_getFrame;
    pHandle->mppAi_releaseFrame             = mppAi_releaseFrame;
    pHandle->mppAi_getRawFrame              = mppAi_getRawFrame;
    pHandle->mppAi_releaseRawFrame          = mppAi_releaseRawFrame;
    pHandle->mppAi_whether_enable_vqe       = mppAi_whether_enable_vqe;
    pHandle->mppAi_whether_enable_vqe_rnr   = mppAi_whether_enable_vqe_rnr;
    pHandle->mppAi_set_track_mode           = mppAi_set_track_mode;

    return pHandle;
}

void mppAi_release(HiAi_S *pHandle)
{
    if (pHandle)
    {
        free(pHandle);
        pHandle = NULL;
    }
}