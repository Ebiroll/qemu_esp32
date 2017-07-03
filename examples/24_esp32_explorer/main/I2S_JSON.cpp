#include <stdlib.h>
#include <JSON.h>
extern "C" {
	#include <soc/i2s_struct.h>
}

JsonObject I2S_JSON() {
	JsonObject obj = JSON::createObject();
	JsonObject tmpObj = JSON::createObject();
	tmpObj.setInt("tx_reset", I2S0.conf.tx_reset);
	tmpObj.setInt("rx_reset", I2S0.conf.rx_reset);
	tmpObj.setInt("tx_start", I2S0.conf.tx_start);
	tmpObj.setInt("rx_start", I2S0.conf.rx_start);
	tmpObj.setInt("tx_slave_mod", I2S0.conf.tx_slave_mod);
	tmpObj.setInt("rx_slave_mod", I2S0.conf.rx_slave_mod);
	tmpObj.setInt("tx_right_first", I2S0.conf.tx_right_first);
	tmpObj.setInt("rx_right_first", I2S0.conf.rx_fifo_reset);
	tmpObj.setInt("tx_msb_shift", I2S0.conf.tx_msb_shift);
	tmpObj.setInt("rx_msb_shift", I2S0.conf.rx_msb_shift);
	tmpObj.setInt("tx_short_sync", I2S0.conf.tx_short_sync);
	tmpObj.setInt("rx_short_sync", I2S0.conf.rx_short_sync);
	tmpObj.setInt("tx_mono", I2S0.conf.tx_mono);
	tmpObj.setInt("rx_mono", I2S0.conf.rx_mono);
	tmpObj.setInt("tx_msb_right", I2S0.conf.tx_msb_right);
	tmpObj.setInt("rx_msb_right", I2S0.conf.rx_msb_right);
	tmpObj.setInt("sig_loopback", I2S0.conf.sig_loopback);
	obj.setObject("conf", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("rx_take_data", I2S0.int_raw.rx_take_data);
	tmpObj.setInt("tx_put_data", I2S0.int_raw.tx_put_data);
	tmpObj.setInt("rx_wfull", I2S0.int_raw.rx_wfull);
	tmpObj.setInt("rx_rempty", I2S0.int_raw.rx_rempty);
	tmpObj.setInt("tx_wfull", I2S0.int_raw.tx_wfull);
	tmpObj.setInt("rx_hung", I2S0.int_raw.rx_hung);
	tmpObj.setInt("tx_hung", I2S0.int_raw.tx_hung);
	tmpObj.setInt("in_done", I2S0.int_raw.in_done);
	tmpObj.setInt("in_suc_eof", I2S0.int_raw.in_suc_eof);
	tmpObj.setInt("in_err_eof", I2S0.int_raw.in_err_eof);
	tmpObj.setInt("out_done", I2S0.int_raw.out_done);
	tmpObj.setInt("out_eof", I2S0.int_raw.out_eof);
	tmpObj.setInt("in_dscr_err", I2S0.int_raw.in_dscr_err);
	tmpObj.setInt("out_dscr_err", I2S0.int_raw.out_dscr_err);
	tmpObj.setInt("in_dscr_empty", I2S0.int_raw.in_dscr_empty);
	tmpObj.setInt("out_total_eof", I2S0.int_raw.out_total_eof);
	obj.setObject("int_raw", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("rx_take_data", I2S0.int_st.rx_take_data);
	tmpObj.setInt("tx_put_data", I2S0.int_st.tx_put_data);
	tmpObj.setInt("rx_wfull", I2S0.int_st.rx_wfull);
	tmpObj.setInt("rx_rempty", I2S0.int_st.rx_rempty);
	tmpObj.setInt("tx_wfull", I2S0.int_st.tx_wfull);
	tmpObj.setInt("rx_hung", I2S0.int_st.rx_hung);
	tmpObj.setInt("tx_hung", I2S0.int_st.tx_hung);
	tmpObj.setInt("in_done", I2S0.int_st.in_done);
	tmpObj.setInt("in_suc_eof", I2S0.int_st.in_suc_eof);
	tmpObj.setInt("in_err_eof", I2S0.int_st.in_err_eof);
	tmpObj.setInt("out_done", I2S0.int_st.out_done);
	tmpObj.setInt("out_eof", I2S0.int_st.out_eof);
	tmpObj.setInt("in_dscr_err", I2S0.int_st.in_dscr_err);
	tmpObj.setInt("out_dscr_err", I2S0.int_st.out_dscr_err);
	tmpObj.setInt("in_dscr_empty", I2S0.int_st.in_dscr_empty);
	tmpObj.setInt("out_total_eof", I2S0.int_st.out_total_eof);
	obj.setObject("int_st", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("rx_take_data", I2S0.int_ena.rx_take_data);
	tmpObj.setInt("tx_put_data", I2S0.int_ena.tx_put_data);
	tmpObj.setInt("rx_wfull", I2S0.int_ena.rx_wfull);
	tmpObj.setInt("rx_rempty", I2S0.int_ena.rx_rempty);
	tmpObj.setInt("tx_wfull", I2S0.int_ena.tx_wfull);
	tmpObj.setInt("rx_hung", I2S0.int_ena.rx_hung);
	tmpObj.setInt("tx_hung", I2S0.int_ena.tx_hung);
	tmpObj.setInt("in_done", I2S0.int_ena.in_done);
	tmpObj.setInt("in_suc_eof", I2S0.int_ena.in_suc_eof);
	tmpObj.setInt("in_err_eof", I2S0.int_ena.in_err_eof);
	tmpObj.setInt("out_done", I2S0.int_ena.out_done);
	tmpObj.setInt("out_eof", I2S0.int_ena.out_eof);
	tmpObj.setInt("in_dscr_err", I2S0.int_ena.in_dscr_err);
	tmpObj.setInt("out_dscr_err", I2S0.int_ena.out_dscr_err);
	tmpObj.setInt("in_dscr_empty", I2S0.int_ena.in_dscr_empty);
	tmpObj.setInt("out_total_eof", I2S0.int_ena.out_total_eof);
	obj.setObject("int_ena", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("take_data", I2S0.int_clr.take_data);
	tmpObj.setInt("put_data", I2S0.int_clr.put_data);
	tmpObj.setInt("rx_wfull", I2S0.int_clr.rx_wfull);
	tmpObj.setInt("rx_rempty", I2S0.int_clr.rx_rempty);
	tmpObj.setInt("tx_wfull", I2S0.int_clr.tx_wfull);
	tmpObj.setInt("rx_hung", I2S0.int_clr.rx_hung);
	tmpObj.setInt("tx_hung", I2S0.int_clr.tx_hung);
	tmpObj.setInt("in_done", I2S0.int_clr.in_done);
	tmpObj.setInt("in_suc_eof", I2S0.int_clr.in_suc_eof);
	tmpObj.setInt("in_err_eof", I2S0.int_clr.in_err_eof);
	tmpObj.setInt("out_done", I2S0.int_clr.out_done);
	tmpObj.setInt("out_eof", I2S0.int_clr.out_eof);
	tmpObj.setInt("in_dscr_err", I2S0.int_clr.in_dscr_err);
	tmpObj.setInt("out_dscr_err", I2S0.int_clr.out_dscr_err);
	tmpObj.setInt("in_dscr_empty", I2S0.int_clr.in_dscr_empty);
	tmpObj.setInt("out_total_eof", I2S0.int_clr.out_total_eof);
	obj.setObject("int_clr", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("tx_bck_in_delay", I2S0.timing.tx_bck_in_delay);
	tmpObj.setInt("tx_ws_in_delay", I2S0.timing.tx_ws_in_delay);
	tmpObj.setInt("rx_bck_in_delay", I2S0.timing.rx_bck_in_delay);
	tmpObj.setInt("rx_ws_in_delay", I2S0.timing.rx_ws_in_delay);
	tmpObj.setInt("rx_sd_in_delay", I2S0.timing.rx_sd_in_delay);
	tmpObj.setInt("tx_bck_out_delay", I2S0.timing.tx_bck_out_delay);
	tmpObj.setInt("tx_ws_out_delay", I2S0.timing.tx_ws_out_delay);
	tmpObj.setInt("tx_sd_out_delay", I2S0.timing.tx_sd_out_delay);
	tmpObj.setInt("rx_ws_out_delay", I2S0.timing.rx_ws_out_delay);
	tmpObj.setInt("rx_bck_out_delay", I2S0.timing.rx_bck_out_delay);
	tmpObj.setInt("tx_dsync_sw", I2S0.timing.tx_dsync_sw);
	tmpObj.setInt("rx_dsync_sw", I2S0.timing.rx_dsync_sw);
	tmpObj.setInt("data_enable_delay", I2S0.timing.data_enable_delay);
	tmpObj.setInt("tx_bck_in_inv", I2S0.timing.tx_bck_in_inv);
	obj.setObject("timing", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("rx_data_num", I2S0.fifo_conf.rx_data_num);
	tmpObj.setInt("tx_data_num", I2S0.fifo_conf.tx_data_num);
	tmpObj.setInt("dscr_en", I2S0.fifo_conf.dscr_en);
	tmpObj.setInt("tx_fifo_mod", I2S0.fifo_conf.tx_fifo_mod);
	tmpObj.setInt("rx_fifo_mod", I2S0.fifo_conf.rx_fifo_mod);
	tmpObj.setInt("tx_fifo_mod_force_en", I2S0.fifo_conf.tx_fifo_mod_force_en);
	tmpObj.setInt("rx_fifo_mod_force_en", I2S0.fifo_conf.rx_fifo_mod_force_en);
	obj.setObject("fifo_conf", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("tx_chan_mod", I2S0.conf_chan.tx_chan_mod);
	tmpObj.setInt("rx_chan_mod", I2S0.conf_chan.rx_chan_mod);
	obj.setObject("conf_chan", tmpObj);


	tmpObj = JSON::createObject();
	tmpObj.setInt("addr", I2S0.out_link.addr);
	tmpObj.setInt("stop", I2S0.out_link.stop);
	tmpObj.setInt("start", I2S0.out_link.start);
	tmpObj.setInt("restart", I2S0.out_link.restart);
	tmpObj.setInt("park", I2S0.out_link.park);
	obj.setObject("out_link", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("addr", I2S0.in_link.addr);
	tmpObj.setInt("stop", I2S0.in_link.stop);
	tmpObj.setInt("start", I2S0.in_link.start);
	tmpObj.setInt("restart", I2S0.in_link.restart);
	tmpObj.setInt("park", I2S0.in_link.park);
	obj.setObject("in_link", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("mode", I2S0.ahb_test.mode);
	tmpObj.setInt("addr", I2S0.ahb_test.addr);
	obj.setObject("ahb_test", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("in_rst", I2S0.lc_conf.in_rst);
	tmpObj.setInt("out_rst", I2S0.lc_conf.out_rst);
	tmpObj.setInt("ahbm_fifo_rst", I2S0.lc_conf.ahbm_fifo_rst);
	tmpObj.setInt("ahbm_rst", I2S0.lc_conf.ahbm_rst);
	tmpObj.setInt("out_loop_test", I2S0.lc_conf.out_loop_test);
	tmpObj.setInt("in_loop_test", I2S0.lc_conf.in_loop_test);
	tmpObj.setInt("out_auto_wrback", I2S0.lc_conf.out_auto_wrback);
	tmpObj.setInt("out_no_restart_clr", I2S0.lc_conf.out_no_restart_clr);
	tmpObj.setInt("out_eof_mode", I2S0.lc_conf.out_eof_mode);
	tmpObj.setInt("outdscr_burst_en", I2S0.lc_conf.outdscr_burst_en);
	tmpObj.setInt("indscr_burst_en", I2S0.lc_conf.indscr_burst_en);
	tmpObj.setInt("out_data_burst_en", I2S0.lc_conf.out_data_burst_en);
	tmpObj.setInt("check_owner", I2S0.lc_conf.check_owner);
	tmpObj.setInt("mem_trans_en", I2S0.lc_conf.mem_trans_en);
	obj.setObject("lc_conf", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("wdata", I2S0.out_fifo_push.wdata);
	tmpObj.setInt("push", I2S0.out_fifo_push.push);
	obj.setObject("out_fifo_push", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("rdata", I2S0.in_fifo_pop.rdata);
	tmpObj.setInt("pop", I2S0.in_fifo_pop.pop);
	obj.setObject("in_fifo_pop", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("fifo_timeout", I2S0.lc_hung_conf.fifo_timeout);
	tmpObj.setInt("fifo_timeout_shift", I2S0.lc_hung_conf.fifo_timeout_shift);
	tmpObj.setInt("fifo_timeout_ena", I2S0.lc_hung_conf.fifo_timeout_ena);
	obj.setObject("lc_hung_conf", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("y_max", I2S0.cvsd_conf0.y_max);
	tmpObj.setInt("y_min", I2S0.cvsd_conf0.y_min);
	obj.setObject("cvsd_conf0", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("sigma_max", I2S0.cvsd_conf1.sigma_max);
	tmpObj.setInt("sigma_min", I2S0.cvsd_conf1.sigma_min);
	obj.setObject("cvsd_conf1", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("cvsd_k", I2S0.cvsd_conf2.cvsd_k);
	tmpObj.setInt("cvsd_j", I2S0.cvsd_conf2.cvsd_j);
	tmpObj.setInt("cvsd_beta", I2S0.cvsd_conf2.cvsd_beta);
	tmpObj.setInt("cvsd_h", I2S0.cvsd_conf2.cvsd_h);
	obj.setObject("cvsd_conf2", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("good_pack_max", I2S0.plc_conf0.good_pack_max);
	tmpObj.setInt("n_err_seg", I2S0.plc_conf0.n_err_seg);
	tmpObj.setInt("shift_rate", I2S0.plc_conf0.shift_rate);
	tmpObj.setInt("max_slide_sample", I2S0.plc_conf0.max_slide_sample);
	tmpObj.setInt("pack_len_8k", I2S0.plc_conf0.pack_len_8k);
	tmpObj.setInt("n_min_err", I2S0.plc_conf0.n_min_err);
	obj.setObject("plc_conf0", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("bad_cef_atten_para", I2S0.plc_conf1.bad_cef_atten_para);
	tmpObj.setInt("bad_cef_atten_para_shift", I2S0.plc_conf1.bad_cef_atten_para_shift);
	tmpObj.setInt("bad_ola_win2_para_shift", I2S0.plc_conf1.bad_ola_win2_para_shift);
	tmpObj.setInt("bad_ola_win2_para", I2S0.plc_conf1.bad_ola_win2_para);
	tmpObj.setInt("slide_win_len", I2S0.plc_conf1.slide_win_len);
	obj.setObject("plc_conf1", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("cvsd_seg_mod", I2S0.plc_conf2.cvsd_seg_mod);
	tmpObj.setInt("min_period", I2S0.plc_conf2.min_period);
	obj.setObject("plc_conf2", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("en", I2S0.esco_conf0.en);
	tmpObj.setInt("chan_mod", I2S0.esco_conf0.chan_mod);
	tmpObj.setInt("cvsd_dec_pack_err", I2S0.esco_conf0.cvsd_dec_pack_err);
	tmpObj.setInt("cvsd_pack_len_8k", I2S0.esco_conf0.cvsd_pack_len_8k);
	tmpObj.setInt("cvsd_inf_en", I2S0.esco_conf0.cvsd_inf_en);
	tmpObj.setInt("cvsd_dec_start", I2S0.esco_conf0.cvsd_dec_start);
	tmpObj.setInt("cvsd_dec_reset", I2S0.esco_conf0.cvsd_dec_reset);
	tmpObj.setInt("plc_en", I2S0.esco_conf0.plc_en);
	tmpObj.setInt("plc2dma_en", I2S0.esco_conf0.plc2dma_en);
	obj.setObject("esco_conf0", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("with_en", I2S0.sco_conf0.with_en);
	tmpObj.setInt("no_en", I2S0.sco_conf0.no_en);
	tmpObj.setInt("cvsd_enc_start", I2S0.sco_conf0.cvsd_enc_start);
	tmpObj.setInt("cvsd_enc_reset", I2S0.sco_conf0.cvsd_enc_reset);
	obj.setObject("sco_conf0", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("tx_pcm_conf", I2S0.conf1.tx_pcm_conf);
	tmpObj.setInt("tx_pcm_bypass", I2S0.conf1.tx_pcm_bypass);
	tmpObj.setInt("rx_pcm_conf", I2S0.conf1.rx_pcm_conf);
	tmpObj.setInt("rx_pcm_bypass", I2S0.conf1.rx_pcm_bypass);
	tmpObj.setInt("tx_stop_en", I2S0.conf1.tx_stop_en);
	tmpObj.setInt("tx_zeros_rm_en", I2S0.conf1.tx_zeros_rm_en);
	obj.setObject("conf1", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("fifo_force_pd", I2S0.pd_conf.fifo_force_pd);
	tmpObj.setInt("fifo_force_pu", I2S0.pd_conf.fifo_force_pu);
	tmpObj.setInt("plc_mem_force_pd", I2S0.pd_conf.plc_mem_force_pd);
	tmpObj.setInt("plc_mem_force_pu", I2S0.pd_conf.plc_mem_force_pu);
	obj.setObject("pd_conf", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("camera_en", I2S0.conf2.camera_en);
	tmpObj.setInt("lcd_tx_wrx2_en", I2S0.conf2.lcd_tx_wrx2_en);
	tmpObj.setInt("lcd_tx_sdx2_en", I2S0.conf2.lcd_tx_sdx2_en);
	tmpObj.setInt("data_enable_test_en", I2S0.conf2.data_enable_test_en);
	tmpObj.setInt("data_enable", I2S0.conf2.data_enable);
	tmpObj.setInt("lcd_en", I2S0.conf2.lcd_en);
	tmpObj.setInt("ext_adc_start_en", I2S0.conf2.ext_adc_start_en);
	tmpObj.setInt("inter_valid_en", I2S0.conf2.inter_valid_en);
	obj.setObject("conf2", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("clkm_div_num", I2S0.clkm_conf.clkm_div_num);
	tmpObj.setInt("clkm_div_b", I2S0.clkm_conf.clkm_div_b);
	tmpObj.setInt("clkm_div_a", I2S0.clkm_conf.clkm_div_a);
	tmpObj.setInt("clk_en", I2S0.clkm_conf.clk_en);
	tmpObj.setInt("clka_en", I2S0.clkm_conf.clka_en);
	obj.setObject("clkm_conf", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("tx_bck_div_num", I2S0.sample_rate_conf.tx_bck_div_num);
	tmpObj.setInt("rx_bck_div_num", I2S0.sample_rate_conf.rx_bck_div_num);
	tmpObj.setInt("tx_bits_mod", I2S0.sample_rate_conf.tx_bits_mod);
	tmpObj.setInt("rx_bits_mod", I2S0.sample_rate_conf.rx_bits_mod);
	obj.setObject("sample_rate_conf", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("tx_pdm_en", I2S0.pdm_conf.tx_pdm_en);
	tmpObj.setInt("rx_pdm_en", I2S0.pdm_conf.rx_pdm_en);
	tmpObj.setInt("pcm2pdm_conv_en", I2S0.pdm_conf.pcm2pdm_conv_en);
	tmpObj.setInt("pdm2pcm_conv_en", I2S0.pdm_conf.pdm2pcm_conv_en);
	tmpObj.setInt("tx_sinc_osr2", I2S0.pdm_conf.tx_sinc_osr2);
	tmpObj.setInt("tx_prescale", I2S0.pdm_conf.tx_prescale);
	tmpObj.setInt("tx_hp_in_shift", I2S0.pdm_conf.tx_hp_in_shift);
	tmpObj.setInt("tx_lp_in_shift", I2S0.pdm_conf.tx_lp_in_shift);
	tmpObj.setInt("tx_sinc_in_shift", I2S0.pdm_conf.tx_sinc_in_shift);
	tmpObj.setInt("tx_sigmadelta_in_shift", I2S0.pdm_conf.tx_sigmadelta_in_shift);
	tmpObj.setInt("rx_sinc_dsr_16_en", I2S0.pdm_conf.rx_sinc_dsr_16_en);
	tmpObj.setInt("txhp_bypass", I2S0.pdm_conf.txhp_bypass);
	obj.setObject("pdm_conf", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("tx_pdm_fs", I2S0.pdm_freq_conf.tx_pdm_fs);
	tmpObj.setInt("tx_pdm_fp", I2S0.pdm_freq_conf.tx_pdm_fp);
	obj.setObject("pdm_freq_conf", tmpObj);

	tmpObj = JSON::createObject();
	tmpObj.setInt("tx_idle", I2S0.state.tx_idle);
	tmpObj.setInt("tx_fifo_reset_back", I2S0.state.tx_fifo_reset_back);
	tmpObj.setInt("rx_fifo_reset_back", I2S0.state.rx_fifo_reset_back);
	obj.setObject("state", tmpObj);

	return obj;
}
