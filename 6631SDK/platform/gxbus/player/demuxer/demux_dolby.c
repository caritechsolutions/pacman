/*
*  This file is part of MPlayer.
*
*  MPlayer is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  MPlayer is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along
*  with MPlayer; if not, write to the Free Software Foundation, Inc.,
*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "gx_demux.h"
#include "gx_common.h"
#include "stheader.h"

#define DOLBY_UNIT (64*1024)

#define CONFIG_INSIDE 0

typedef struct
{
	int b_exist 	;			//-b
	int pcmwordtype ;
	int c_exist 	;			//-c
	int kmode       ;
	int d_exist 	;			//-d
	int f_exist 	;			//-f
	int f           ;
	int dp_exist    ;
	int p           ;
	int h_exist 	;			//-h
	int i_exist 	;			//-i
	int k_exist 	;			//-k
	int compmode    ;
	int l_exist 	;			//-l
	int outlfe      ;
	int m_exist 	;			//-m
	int mr_exist	;			//-mr
	int m           ;
	int n_exist 	;			//-n
    int outnchans   ;
    int o_exist 	;			//-o
    int oc_exist    ;
    int op_exist    ;
    int od_exist    ;
    int p_exist 	;			//-p
    int pcmscale	;			//((x + 0.005) * 100)
    int q_exist 	;			//-q
    int r_exist 	;			//-r
    int framestart  ;
    int frameend    ;
    int s_exist 	;			//-s
    int stereomode  ;
    int u_exist 	;			//-u
    int dualmode    ;
    int v_exist 	;			//-v
    int verbose 	;
    int w_exist 	;			//-w
    int upsample    ;
    int x_exist 	;			//-x
    int dynscalehigh;			//((x + 0.005) * 100)
    int y_exist 	;			//-y
    int dynscalelow	;			//((x + 0.005) * 100);
    int z_exist 	;			//-z
    int zp_exist    ;
    int zd_exist    ;
    int num_exist 	;			//how much -0~7
    int chanval[8]  ;
	int routing_index[8];
    int byterev;	// 1--little endian,
    int ddoutbaseaddr;
    int ddoutlength;
    int ddoutid;
} dolby_info_header;


typedef struct
{
	char header[sizeof(dolby_info_header)];
	char buf[DOLBY_UNIT];
	int  format;
	int64_t duration;
    int big_endian;
} DemuxDolbyPriv;



#define LITTLE_TO_CPU(a)	((((a) & 0xff) << 24) | ((((a) >> 8) & 0xff) << 16) \
							| ((((a) >> 16) & 0xff) << 8) | (((a) >> 24) & 0xff))
static int aa2i(char * aa)
{
	int a0=0;
	int read_cnt=0;
		while((aa[read_cnt]!='-')&&(aa[read_cnt]!=' ')&&(aa[read_cnt]!='	')&&(aa[read_cnt]!=':'))
		{

		a0= a0*10 + aa[read_cnt] - '0';
		read_cnt++;

		}
	return a0;
}

static float aa2f(char * aa)
{
	float a0=0,a1=0.0;
	int read_cnt=0;
	int point_exist=0;
	int point_add=0;
	int temp,temp1;
	while((aa[read_cnt]!='-')&&(aa[read_cnt]!=' ')&&(aa[read_cnt]!='	'))
	{
	if(aa[read_cnt]=='.')
	{
		point_exist = 1;
		read_cnt++;
		continue;
	}
	if(point_exist)
	{
		point_add++;
		temp= aa[read_cnt] - '0';
		temp1=point_add;
		a1 = (float)temp;
		while(temp1--)
			a1=a1/10.0;
		a0 = a0 + a1;
		read_cnt++;
	}
	else
	{
		a0= a0*10 + aa[read_cnt] - '0';
		read_cnt++;
	}

	}
	return a0;
}


#if CONFIG_INSIDE
static char cc[][512]=
{
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ichanfigchanfigswp_0_7.ec3 -opDCVchanfig_0_7chanfigswp_0_7.pcm -zd",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust07_compr_cust0_x00y00.pcm -zd   ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust07_compr_cust0_x03y03.pcm -zd   ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust07_compr_cust0_x05y05.pcm -zd   ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust07_compr_cust0_x07y07.pcm -zd   ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust07_compr_cust0_x10y10.pcm -zd   ",
"dcv.exe -b0 -k0 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr7_compr.ec3 -opDCVcompr_cust07_compr_cust0_LoRo.pcm -zd                     ",
"dcv.exe -b0 -k1 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust17_compr_cust1_x00y00.pcm -zd   ",
"dcv.exe -b0 -k1 -l1 -m7 -n6 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust17_compr_cust1_x03y03.pcm -zd   ",
"dcv.exe -b0 -k1 -l1 -m7 -n6 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust17_compr_cust1_x05y05.pcm -zd   ",
"dcv.exe -b0 -k1 -l1 -m7 -n6 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust17_compr_cust1_x07y07.pcm -zd   ",
"dcv.exe -b0 -k1 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_cust17_compr_cust1_x10y10.pcm -zd   ",
"dcv.exe -b0 -k1 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr7_compr.ec3 -opDCVcompr_cust17_compr_cust1_LoRo.pcm -zd                     ",
"dcv.exe -b0 -k3 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icomprdrc.ec3 -opDCVcompr_drcdrc.pcm -zd                          ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_line7_compr_line_x00y00.pcm -zd     ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_line7_compr_line_x03y03.pcm -zd     ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_line7_compr_line_x05y05.pcm -zd     ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_line7_compr_line_x07y07.pcm -zd     ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_line7_compr_line_x10y10.pcm -zd     ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr7_compr.ec3 -opDCVcompr_line7_compr_line_LoRo.pcm -zd                       ",
"dcv.exe -b0 -k3 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_rf7_compr_rf_x00y00.pcm -zd         ",
"dcv.exe -b0 -k3 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -opDCVcompr_rf7_compr_rf_x10y10.pcm -zd         ",
"dcv.exe -b0 -k3 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr7_compr.ec3 -opDCVcompr_rf7_compr_rf_LoRo.pcm -zd                           ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ichanfigchanfigswp_0_7.ec3 -odDCVconv_acmodchanfigswp_0_7.ac3 -zp ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -odDCVconv_compr7_compr_line.ac3 -zp            ",
"dcv.exe -b0 -k3 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr7_compr.ec3 -odDCVconv_compr7_compr_rf.ac3 -zp              ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -idialnorm7_dnswp.ec3 -odDCVconv_dialnorm7_dnswp.ac3 -zp           ",
"dcv.exe -b0 -k2 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmixdownmix.ec3 -odDCVconv_downmixdownmix_LoRo.ac3 -zp                        ",
"dcv.exe -b0 -k2 -l0 -m2 -n2 -s1 -x0.0 -y0.0 -0L -1R -idownmixdownmix.ec3 -odDCVconv_downmixdownmix_LtRt.ac3 -zp                        ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmonodualmono.ec3 -odDCVconv_dualmonodualmono.ac3 -zp -u0                     ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -isampleratesmpl32.ec3 -odDCVconv_sampleratesmpl32.ac3 -zp         ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -isampleratesmpl44.ec3 -odDCVconv_sampleratesmpl44.ac3 -zp         ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -isampleratesmpl48.ec3 -odDCVconv_sampleratesmpl48.ac3 -zp         ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icouplingcpl_off.ec3 -opDCVcouplingcpl_off.pcm -zd                ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icouplingcpl_on.ec3 -opDCVcouplingcpl_on.pcm -zd                  ",
"dcv.exe -b0 -k1 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idialnorm7_dnswp.ec3 -opDCVdec_dialnorm7_dnswp.pcm -zd            ",
"dcv.exe -b0 -k2 -l0 -m2 -n2 -s0 -x0.0 -y0.0 -0L -1R -idownmixdownmix.ec3 -opDCVdec_downmixdownmix_pref.pcm -zd                         ",
"dcv.exe -b0 -k2 -l0 -m2 -n2 -s1 -x0.0 -y0.0 -0L -1R -idownmixdownmix.ec3 -opDCVdec_downmixdownmix_LtRt.pcm -zd                         ",
"dcv.exe -b0 -k2 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmixdownmix.ec3 -opDCVdec_downmixdownmix_LoRo.pcm -zd                         ",
"dcv.exe -b0 -k2 -l0 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmixdownmix.ec3 -opDCVdec_downmixdownmix_32.pcm -zd           ",
"dcv.exe -b0 -k2 -l0 -m6 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmixdownmix.ec3 -opDCVdec_downmixdownmix_22.pcm -zd           ",
"dcv.exe -b0 -k2 -l0 -m5 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmixdownmix.ec3 -opDCVdec_downmixdownmix_31.pcm -zd           ",
"dcv.exe -b0 -k2 -l0 -m4 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmixdownmix.ec3 -opDCVdec_downmixdownmix_21.pcm -zd           ",
"dcv.exe -b0 -k2 -l0 -m3 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmixdownmix.ec3 -opDCVdec_downmixdownmix_30.pcm -zd           ",
"dcv.exe -b0 -k2 -l0 -m1 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmixdownmix.ec3 -opDCVdec_downmixdownmix_10.pcm -zd           ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmonodualmono.ec3 -opDCVdec_dualmonodualmono_LoRo_Stereo.pcm -zd -u0          ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmonodualmono.ec3 -opDCVdec_dualmonodualmono_LoRo_LMono.pcm -zd -u1           ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmonodualmono.ec3 -opDCVdec_dualmonodualmono_LoRo_RMono.pcm -zd -u2           ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmonodualmono.ec3 -opDCVdec_dualmonodualmono_LoRo_MixedMono.pcm -zd -u3       ",
"dcv.exe -b0 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmonodualmono.ec3 -opDCVdec_dualmonodualmono_10_Stereo.pcm -zd -u0                ",
"dcv.exe -b0 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmonodualmono.ec3 -opDCVdec_dualmonodualmono_10_LMono.pcm -zd -u1                 ",
"dcv.exe -b0 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmonodualmono.ec3 -opDCVdec_dualmonodualmono_10_RMono.pcm -zd -u2                 ",
"dcv.exe -b0 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmonodualmono.ec3 -opDCVdec_dualmonodualmono_10_MixedMono.pcm -zd -u3             ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isampleratesmpl32.ec3 -opDCVdec_sampleratesmpl32.pcm -zd          ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isampleratesmpl44.ec3 -opDCVdec_sampleratesmpl44.pcm -zd          ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isampleratesmpl48.ec3 -opDCVdec_sampleratesmpl48.pcm -zd          ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idynrng7_dynrng_16.ec3 -opDCVdynrng7_dynrng_16.pcm -zd            ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ifreqresp_thdfrq7_frqstp.ec3 -opDCVfreqresp7_frqstp.pcm -zd       ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara1.ec3 -opDCVkaraokekara1_LtRt_None.pcm -zd -c0                        ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara2.ec3 -opDCVkaraokekara2_LtRt_None.pcm -zd -c0                        ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara3.ec3 -opDCVkaraokekara3_LtRt_None.pcm -zd -c0                        ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraokekara1.ec3 -opDCVkaraokekara1_51_None.pcm -zd -c0          ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara1.ec3 -opDCVkaraokekara1_LtRt_V1.pcm -zd -c1                          ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara2.ec3 -opDCVkaraokekara2_LtRt_V1.pcm -zd -c1                          ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara3.ec3 -opDCVkaraokekara3_LtRt_V1.pcm -zd -c1                          ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraokekara1.ec3 -opDCVkaraokekara1_51_V1.pcm -zd -c1            ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara1.ec3 -opDCVkaraokekara1_LtRt_V2.pcm -zd -c2                          ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara2.ec3 -opDCVkaraokekara2_LtRt_V2.pcm -zd -c2                          ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara3.ec3 -opDCVkaraokekara3_LtRt_V2.pcm -zd -c2                          ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraokekara1.ec3 -opDCVkaraokekara1_51_V2.pcm -zd -c2            ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara1.ec3 -opDCVkaraokekara1_LtRt_V1V2.pcm -zd -c3                        ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara2.ec3 -opDCVkaraokekara2_LtRt_V1V2.pcm -zd -c3                        ",
"dcv.exe -b0 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraokekara3.ec3 -opDCVkaraokekara3_LtRt_V1V2.pcm -zd -c3                        ",
"dcv.exe -b0 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraokekara1.ec3 -opDCVkaraokekara1_51_V1V2.pcm -zd -c3          ",
"dcv.exe -b0 -k2 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmixlfe_mixlevs.ec3 -opDCVlfe_mixlevslfe_mixlevs_line.pcm -zd                 ",
"dcv.exe -b0 -k3 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmixlfe_mixlevs.ec3 -opDCVlfe_mixlevslfe_mixlevs_rf.pcm -zd                   ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -imultitone7_mult60_16.ec3 -opDCVmultitone7_mult60_16.pcm -zd      ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ispxspx.ec3 -opDCVspxspx.pcm -zd                                  ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ifreqresp_thdfrq7_frqstp.ec3 -opDCVthdfrq7_thdfrq.pcm -zd         ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ithdlvl7_aswp4k_16.ec3 -opDCVthdlvl7_aswp4k_16.pcm -zd            ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -itpdf7_tpdf_16.ec3 -opDCVtpdf7_tpdf_16.pcm -zd                    ",
"dcv.exe -b0 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -itpnptpnp.ec3 -opDCVtpnptpnp.pcm -zd                              ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ichanfig_chanfigswp_0_7.ec3 -opchanfig_0_7_chanfigswp_0_7.pcm 						",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ichanfig_chanfigswp_8_27.ec3 -opchanfig_8_27_chanfigswp_8_27.pcm           ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x00y00.pcm                 ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x03y03.pcm                 ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x05y05.pcm                 ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x07y07.pcm                 ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x10y10.pcm                 ",
"dec71.exe -b24 -k0 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_LoRo.pcm                                             ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x00y00_51ch.pcm                      ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x03y03_51ch.pcm                      ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x05y05_51ch.pcm                      ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x07y07_51ch.pcm                      ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x10y10_51ch.pcm                      ",
"dec71.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x00y00.pcm                 ",
"dec71.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x03y03.pcm                 ",
"dec71.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x05y05.pcm                 ",
"dec71.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x07y07.pcm                 ",
"dec71.exe -b24 -k1 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x10y10.pcm                 ",
"dec71.exe -b24 -k1 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_LoRo.pcm                                             ",
"dec71.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x00y00_51ch.pcm                      ",
"dec71.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x03y03_51ch.pcm                      ",
"dec71.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x05y05_51ch.pcm                      ",
"dec71.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x07y07_51ch.pcm                      ",
"dec71.exe -b24 -k1 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x10y10_51ch.pcm                      ",
"dec71.exe -b24 -k3 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_drc.ec3 -opcompr_drc_drc.pcm                                        ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x00y00.pcm                   ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x03y03.pcm                   ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x05y05.pcm                   ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x07y07.pcm                   ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x10y10.pcm                   ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr_7_compr.ec3 -opcompr_line_7_compr_line_LoRo.pcm                                               ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x00y00_51ch.pcm                        ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x03y03_51ch.pcm                        ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x05y05_51ch.pcm                        ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x07y07_51ch.pcm                        ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x10y10_51ch.pcm                        ",
"dec71.exe -b24 -k3 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_compr_order.ec3 -opcompr_order_compr_order.pcm                      ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_dynrng_order.ec3 -opcompr_order_dynrng_order.pcm                    ",
"dec71.exe -b24 -k3 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_x00y00.pcm                       ",
"dec71.exe -b24 -k3 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_x10y10.pcm                       ",
"dec71.exe -b24 -k3 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_LoRo.pcm                                                   ",
"dec71.exe -b24 -k3 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_x00y00_51ch.pcm                            ",
"dec71.exe -b24 -k3 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_x10y10_51ch.pcm                            ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icoupling_cpl_off.ec3 -opcoupling_cpl_off.pcm                              ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icoupling_cpl_on.ec3 -opcoupling_cpl_on.pcm                                ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icoupling_cpl_off.ec3 -opcoupling_cpl_off_51ch.pcm                                   ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icoupling_cpl_on.ec3 -opcoupling_cpl_on_51ch.pcm                                     ",
"dec71.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -idialnorm_7_dnswp.ec3 -opdec_dialnorm_7_dnswp.pcm                          ",
"dec71.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idialnorm_7_dnswp.ec3 -opdec_dialnorm_7_dnswp_51ch.pcm                               ",
"dec71.exe -b24 -k2 -l0 -m2 -n2 -s0 -x0.0 -y0.0 -0L -1R -idownmix_downmix.ec3 -opdec_downmix_downmix_pref.pcm                                                 ",
"dec71.exe -b24 -k2 -l0 -m2 -n2 -s1 -x0.0 -y0.0 -0L -1R -idownmix_downmix.ec3 -opdec_downmix_downmix_LtRt.pcm                                                 ",
"dec71.exe -b24 -k2 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmix_downmix.ec3 -opdec_downmix_downmix_LoRo.pcm                                                 ",
"dec71.exe -b24 -k2 -l0 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_32.pcm                                   ",
"dec71.exe -b24 -k2 -l0 -m6 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_22.pcm                                   ",
"dec71.exe -b24 -k2 -l0 -m5 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_31.pcm                                   ",
"dec71.exe -b24 -k2 -l0 -m4 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_21.pcm                                   ",
"dec71.exe -b24 -k2 -l0 -m3 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_30.pcm                                   ",
"dec71.exe -b24 -k2 -l0 -m1 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_10.pcm                                   ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_LoRo_Stereo.pcm -u0                                  ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_LoRo_LMono.pcm -u1                                   ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_LoRo_RMono.pcm -u2                                   ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_LoRo_MixedMono.pcm -u3                               ",
"dec71.exe -b24 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_10_Stereo.pcm -u0                                        ",
"dec71.exe -b24 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_10_LMono.pcm -u1                                         ",
"dec71.exe -b24 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_10_RMono.pcm -u2                                         ",
"dec71.exe -b24 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_10_MixedMono.pcm -u3                                     ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isamplerate_smpl32.ec3 -opdec_samplerate_smpl32.pcm                        ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isamplerate_smpl44.ec3 -opdec_samplerate_smpl44.pcm                        ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isamplerate_smpl48.ec3 -opdec_samplerate_smpl48.pcm                        ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isamplerate_smpl32.ec3 -opdec_samplerate_smpl32_51ch.pcm                             ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isamplerate_smpl44.ec3 -opdec_samplerate_smpl44_51ch.pcm                             ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isamplerate_smpl48.ec3 -opdec_samplerate_smpl48_51ch.pcm                             ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -idynrng_7_dynrng_16.ec3 -opdynrng_7_dynrng_16.pcm                          ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idynrng_7_dynrng_16.ec3 -opdynrng_7_dynrng_16_51ch.pcm                               ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ifreqresp_thdfrq_7_frqstp.ec3 -opfreqresp_7_frqstp.pcm                     ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ifreqresp_thdfrq_7_frqstp.ec3 -opfreqresp_7_frqstp_51ch.pcm                          ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara1.ec3 -opkaraoke_kara1_LtRt_None.pcm -c0                                                ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara2.ec3 -opkaraoke_kara2_LtRt_None.pcm -c0                                                ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara3.ec3 -opkaraoke_kara3_LtRt_None.pcm -c0                                                ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraoke_kara1.ec3 -opkaraoke_kara1_51_None.pcm -c0                                  ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara1.ec3 -opkaraoke_kara1_LtRt_V1.pcm -c1                                                  ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara2.ec3 -opkaraoke_kara2_LtRt_V1.pcm -c1                                                  ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara3.ec3 -opkaraoke_kara3_LtRt_V1.pcm -c1                                                  ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraoke_kara1.ec3 -opkaraoke_kara1_51_V1.pcm -c1                                    ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara1.ec3 -opkaraoke_kara1_LtRt_V2.pcm -c2                                                  ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara2.ec3 -opkaraoke_kara2_LtRt_V2.pcm -c2                                                  ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara3.ec3 -opkaraoke_kara3_LtRt_V2.pcm -c2                                                  ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraoke_kara1.ec3 -opkaraoke_kara1_51_V2.pcm -c2                                    ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara1.ec3 -opkaraoke_kara1_LtRt_V1V2.pcm -c3                                                ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara2.ec3 -opkaraoke_kara2_LtRt_V1V2.pcm -c3                                                ",
"dec71.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara3.ec3 -opkaraoke_kara3_LtRt_V1V2.pcm -c3                                                ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraoke_kara1.ec3 -opkaraoke_kara1_51_V1V2.pcm -c3                                  ",
"dec71.exe -b24 -k0 -l2 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ilfe_lfe2_decode_LFEdec_x2.ec3 -oplfechanfig_LFEdec_x2.pcm                 ",
"dec71.exe -b24 -k0 -l2 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ilfe_lfe2_decode_LFEdec_C.ec3 -oplfechanfig_LFEdec_C.pcm                   ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ilfe_lfe2_decode_LFEoutmode.ec3 -oplfeoutmode_LFEoutmode.pcm               ",
"dec71.exe -b24 -k2 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmix_lfe_mixlevs.ec3 -oplfe_mixlevs_lfe_mixlevs_line.pcm                                         ",
"dec71.exe -b24 -k3 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmix_lfe_mixlevs.ec3 -oplfe_mixlevs_lfe_mixlevs_rf.pcm                                           ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -imultitone_7_mult60_16.ec3 -opmultitone_7_mult60_16.pcm                    ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -imultitone_7_mult60_16.ec3 -opmultitone_7_mult60_16_51ch.pcm                         ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ispx_spx.ec3 -opspx_spx.pcm                                                ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ispx_spx.ec3 -opspx_spx_51ch.pcm                                                     ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_src96k.ec3 -opsrc_frqresp_src96k.pcm -w1                              ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_src88k.ec3 -opsrc_frqresp_src88k.pcm -w1                              ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_src64k.ec3 -opsrc_frqresp_src64k.pcm -w1                              ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_src96k.ec3 -opsrc_frqresp_src96k_51ch.pcm -w1                                   ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_src88k.ec3 -opsrc_frqresp_src88k_51ch.pcm -w1                                   ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_src64k.ec3 -opsrc_frqresp_src64k_51ch.pcm -w1                                   ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_srclimit.ec3 -opsrc_limit_srclimit.pcm -w0                            ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_srclimit.ec3 -opsrc_limit_srclimit_51ch.pcm -w0                                 ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_sfsc_1.ec3 -opsrc_sfsc_sfsc_1.pcm -w0                                 ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_sfsc_2.ec3 -opsrc_sfsc_sfsc_2.pcm -w0                                 ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_sfsc_3.ec3 -opsrc_sfsc_sfsc_3.pcm -w0                                 ",
"dec71.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_sfsc_4.ec3 -opsrc_sfsc_sfsc_4.pcm -w0                                 ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_sfsc_1.ec3 -opsrc_sfsc_sfsc_1_51ch.pcm -w0                                      ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_sfsc_2.ec3 -opsrc_sfsc_sfsc_2_51ch.pcm -w0                                      ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_sfsc_3.ec3 -opsrc_sfsc_sfsc_3_51ch.pcm -w0                                      ",
"dec71.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_sfsc_4.ec3 -opsrc_sfsc_sfsc_4_51ch.pcm -w0                                      ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ifreqresp_thdfrq_7_frqstp.ec3 -opthdfrq_7_thdfrq.pcm                       ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ifreqresp_thdfrq_7_frqstp.ec3 -opthdfrq_7_thdfrq_51ch.pcm                            ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ithdlvl_7_aswp4k_16.ec3 -opthdlvl_7_aswp4k_16.pcm                          ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ithdlvl_7_aswp4k_16.ec3 -opthdlvl_7_aswp4k_16_51ch.pcm                               ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -itpdf_7_tpdf_16.ec3 -optpdf_7_tpdf_16.pcm                                  ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -itpdf_7_tpdf_16.ec3 -optpdf_7_tpdf_16_51ch.pcm                                       ",
"dec71.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -itpnp_tpnp.ec3 -optpnp_tpnp.pcm                                            ",
"dec71.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -itpnp_tpnp.ec3 -optpnp_tpnp_51ch.pcm                                                 ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ichanfig_chanfigswp_0_7.ec3 -opchanfig_0_7_chanfigswp_0_7.pcm 						",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ichanfig_chanfigswp_8_27.ec3 -opchanfig_8_27_chanfigswp_8_27.pcm           ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x00y00.pcm                 ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x03y03.pcm                 ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x05y05.pcm                 ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x07y07.pcm                 ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x10y10.pcm                 ",
"dec51.exe -b24 -k0 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_LoRo.pcm                                             ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x00y00_51ch.pcm                      ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x03y03_51ch.pcm                      ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x05y05_51ch.pcm                      ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x07y07_51ch.pcm                      ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust0_7_compr_cust0_x10y10_51ch.pcm                      ",
"dec51.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x00y00.pcm                 ",
"dec51.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x03y03.pcm                 ",
"dec51.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x05y05.pcm                 ",
"dec51.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x07y07.pcm                 ",
"dec51.exe -b24 -k1 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x10y10.pcm                 ",
"dec51.exe -b24 -k1 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_LoRo.pcm                                             ",
"dec51.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x00y00_51ch.pcm                      ",
"dec51.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x03y03_51ch.pcm                      ",
"dec51.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x05y05_51ch.pcm                      ",
"dec51.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x07y07_51ch.pcm                      ",
"dec51.exe -b24 -k1 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_cust1_7_compr_cust1_x10y10_51ch.pcm                      ",
"dec51.exe -b24 -k3 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_drc.ec3 -opcompr_drc_drc.pcm                                        ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x00y00.pcm                   ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x03y03.pcm                   ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x05y05.pcm                   ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x07y07.pcm                   ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x10y10.pcm                   ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr_7_compr.ec3 -opcompr_line_7_compr_line_LoRo.pcm                                               ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x00y00_51ch.pcm                        ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.3 -y0.3 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x03y03_51ch.pcm                        ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.5 -y0.5 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x05y05_51ch.pcm                        ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.7 -y0.7 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x07y07_51ch.pcm                        ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_line_7_compr_line_x10y10_51ch.pcm                        ",
"dec51.exe -b24 -k3 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_compr_order.ec3 -opcompr_order_compr_order.pcm                      ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_dynrng_order.ec3 -opcompr_order_dynrng_order.pcm                    ",
"dec51.exe -b24 -k3 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_x00y00.pcm                       ",
"dec51.exe -b24 -k3 -l1 -mr -n8 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_x10y10.pcm                       ",
"dec51.exe -b24 -k3 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_LoRo.pcm                                                   ",
"dec51.exe -b24 -k3 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_x00y00_51ch.pcm                            ",
"dec51.exe -b24 -k3 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -icompr_7_compr.ec3 -opcompr_rf_7_compr_rf_x10y10_51ch.pcm                            ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icoupling_cpl_off.ec3 -opcoupling_cpl_off.pcm                              ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -icoupling_cpl_on.ec3 -opcoupling_cpl_on.pcm                                ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icoupling_cpl_off.ec3 -opcoupling_cpl_off_51ch.pcm                                   ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -icoupling_cpl_on.ec3 -opcoupling_cpl_on_51ch.pcm                                     ",
"dec51.exe -b24 -k1 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -idialnorm_7_dnswp.ec3 -opdec_dialnorm_7_dnswp.pcm                          ",
"dec51.exe -b24 -k1 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idialnorm_7_dnswp.ec3 -opdec_dialnorm_7_dnswp_51ch.pcm                               ",
"dec51.exe -b24 -k2 -l0 -m2 -n2 -s0 -x0.0 -y0.0 -0L -1R -idownmix_downmix.ec3 -opdec_downmix_downmix_pref.pcm                                                 ",
"dec51.exe -b24 -k2 -l0 -m2 -n2 -s1 -x0.0 -y0.0 -0L -1R -idownmix_downmix.ec3 -opdec_downmix_downmix_LtRt.pcm                                                 ",
"dec51.exe -b24 -k2 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmix_downmix.ec3 -opdec_downmix_downmix_LoRo.pcm                                                 ",
"dec51.exe -b24 -k2 -l0 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_32.pcm                                   ",
"dec51.exe -b24 -k2 -l0 -m6 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_22.pcm                                   ",
"dec51.exe -b24 -k2 -l0 -m5 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_31.pcm                                   ",
"dec51.exe -b24 -k2 -l0 -m4 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_21.pcm                                   ",
"dec51.exe -b24 -k2 -l0 -m3 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_30.pcm                                   ",
"dec51.exe -b24 -k2 -l0 -m1 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idownmix_downmix.ec3 -opdec_downmix_downmix_10.pcm                                   ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_LoRo_Stereo.pcm -u0                                  ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_LoRo_LMono.pcm -u1                                   ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_LoRo_RMono.pcm -u2                                   ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_LoRo_MixedMono.pcm -u3                               ",
"dec51.exe -b24 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_10_Stereo.pcm -u0                                        ",
"dec51.exe -b24 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_10_LMono.pcm -u1                                         ",
"dec51.exe -b24 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_10_RMono.pcm -u2                                         ",
"dec51.exe -b24 -k2 -l1 -m1 -n1 -s0 -x0.0 -y0.0 -0C -idualmono_dualmono.ec3 -opdec_dualmono_dualmono_10_MixedMono.pcm -u3                                     ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isamplerate_smpl32.ec3 -opdec_samplerate_smpl32.pcm                        ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isamplerate_smpl44.ec3 -opdec_samplerate_smpl44.pcm                        ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isamplerate_smpl48.ec3 -opdec_samplerate_smpl48.pcm                        ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isamplerate_smpl32.ec3 -opdec_samplerate_smpl32_51ch.pcm                             ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isamplerate_smpl44.ec3 -opdec_samplerate_smpl44_51ch.pcm                             ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isamplerate_smpl48.ec3 -opdec_samplerate_smpl48_51ch.pcm                             ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -idynrng_7_dynrng_16.ec3 -opdynrng_7_dynrng_16.pcm                          ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -idynrng_7_dynrng_16.ec3 -opdynrng_7_dynrng_16_51ch.pcm                               ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ifreqresp_thdfrq_7_frqstp.ec3 -opfreqresp_7_frqstp.pcm                     ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ifreqresp_thdfrq_7_frqstp.ec3 -opfreqresp_7_frqstp_51ch.pcm                          ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara1.ec3 -opkaraoke_kara1_LtRt_None.pcm -c0                                                ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara2.ec3 -opkaraoke_kara2_LtRt_None.pcm -c0                                                ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara3.ec3 -opkaraoke_kara3_LtRt_None.pcm -c0                                                ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraoke_kara1.ec3 -opkaraoke_kara1_51_None.pcm -c0                                  ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara1.ec3 -opkaraoke_kara1_LtRt_V1.pcm -c1                                                  ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara2.ec3 -opkaraoke_kara2_LtRt_V1.pcm -c1                                                  ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara3.ec3 -opkaraoke_kara3_LtRt_V1.pcm -c1                                                  ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraoke_kara1.ec3 -opkaraoke_kara1_51_V1.pcm -c1                                    ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara1.ec3 -opkaraoke_kara1_LtRt_V2.pcm -c2                                                  ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara2.ec3 -opkaraoke_kara2_LtRt_V2.pcm -c2                                                  ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara3.ec3 -opkaraoke_kara3_LtRt_V2.pcm -c2                                                  ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraoke_kara1.ec3 -opkaraoke_kara1_51_V2.pcm -c2                                    ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara1.ec3 -opkaraoke_kara1_LtRt_V1V2.pcm -c3                                                ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara2.ec3 -opkaraoke_kara2_LtRt_V1V2.pcm -c3                                                ",
"dec51.exe -b24 -k2 -l1 -m2 -n2 -s1 -x1.0 -y1.0 -0L -1R -ikaraoke_kara3.ec3 -opkaraoke_kara3_LtRt_V1V2.pcm -c3                                                ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x1.0 -y1.0 -0L -1C -2R -3l -4r -5s -ikaraoke_kara1.ec3 -opkaraoke_kara1_51_V1V2.pcm -c3                                  ",
"dec51.exe -b24 -k0 -l2 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ilfe_lfe2_decode_LFEdec_x2.ec3 -oplfechanfig_LFEdec_x2.pcm                 ",
"dec51.exe -b24 -k0 -l2 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ilfe_lfe2_decode_LFEdec_C.ec3 -oplfechanfig_LFEdec_C.pcm                   ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ilfe_lfe2_decode_LFEoutmode.ec3 -oplfeoutmode_LFEoutmode.pcm               ",
"dec51.exe -b24 -k2 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmix_lfe_mixlevs.ec3 -oplfe_mixlevs_lfe_mixlevs_line.pcm                                         ",
"dec51.exe -b24 -k3 -l0 -m2 -n2 -s2 -x0.0 -y0.0 -0L -1R -idownmix_lfe_mixlevs.ec3 -oplfe_mixlevs_lfe_mixlevs_rf.pcm                                           ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -imultitone_7_mult60_16.ec3 -opmultitone_7_mult60_16.pcm                    ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -imultitone_7_mult60_16.ec3 -opmultitone_7_mult60_16_51ch.pcm                         ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ispx_spx.ec3 -opspx_spx.pcm                                                ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ispx_spx.ec3 -opspx_spx_51ch.pcm                                                     ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_src96k.ec3 -opsrc_frqresp_src96k.pcm -w1                              ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_src88k.ec3 -opsrc_frqresp_src88k.pcm -w1                              ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_src64k.ec3 -opsrc_frqresp_src64k.pcm -w1                              ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_src96k.ec3 -opsrc_frqresp_src96k_51ch.pcm -w1                                   ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_src88k.ec3 -opsrc_frqresp_src88k_51ch.pcm -w1                                   ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_src64k.ec3 -opsrc_frqresp_src64k_51ch.pcm -w1                                   ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_srclimit.ec3 -opsrc_limit_srclimit.pcm -w0                            ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_srclimit.ec3 -opsrc_limit_srclimit_51ch.pcm -w0                                 ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_sfsc_1.ec3 -opsrc_sfsc_sfsc_1.pcm -w0                                 ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_sfsc_2.ec3 -opsrc_sfsc_sfsc_2.pcm -w0                                 ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_sfsc_3.ec3 -opsrc_sfsc_sfsc_3.pcm -w0                                 ",
"dec51.exe -b24 -k2 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -isrc_sfsc_4.ec3 -opsrc_sfsc_sfsc_4.pcm -w0                                 ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_sfsc_1.ec3 -opsrc_sfsc_sfsc_1_51ch.pcm -w0                                      ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_sfsc_2.ec3 -opsrc_sfsc_sfsc_2_51ch.pcm -w0                                      ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_sfsc_3.ec3 -opsrc_sfsc_sfsc_3_51ch.pcm -w0                                      ",
"dec51.exe -b24 -k2 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -isrc_sfsc_4.ec3 -opsrc_sfsc_sfsc_4_51ch.pcm -w0                                      ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ifreqresp_thdfrq_7_frqstp.ec3 -opthdfrq_7_thdfrq.pcm                       ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ifreqresp_thdfrq_7_frqstp.ec3 -opthdfrq_7_thdfrq_51ch.pcm                            ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -ithdlvl_7_aswp4k_16.ec3 -opthdlvl_7_aswp4k_16.pcm                          ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -ithdlvl_7_aswp4k_16.ec3 -opthdlvl_7_aswp4k_16_51ch.pcm                               ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -itpdf_7_tpdf_16.ec3 -optpdf_7_tpdf_16.pcm                                  ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -itpdf_7_tpdf_16.ec3 -optpdf_7_tpdf_16_51ch.pcm                                       ",
"dec51.exe -b24 -k0 -l1 -mr -n8 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -6x1 -7x2 -itpnp_tpnp.ec3 -optpnp_tpnp.pcm                                            ",
"dec51.exe -b24 -k0 -l1 -m7 -n6 -s0 -x0.0 -y0.0 -0L -1C -2R -3l -4r -5s -itpnp_tpnp.ec3 -optpnp_tpnp_51ch.pcm                                                 ",
};
#else
static char cc[1][512];
#endif

void dolby_parse(char* aa ,dolby_info_header* info_header )
{
	int read_cnt=0;
	char bb;
	int routing_index=0;
	int chanval = 0;
	int temp;
    float	tt_pcmscale,tt_dynscalehigh,tt_dynscalelow;

	info_header->b_exist 		= LITTLE_TO_CPU(1					);	//-b
	info_header->pcmwordtype	= LITTLE_TO_CPU(0					);
	info_header->c_exist 		= LITTLE_TO_CPU(0					);	//-c
	info_header->kmode          = LITTLE_TO_CPU(0					);
	info_header->d_exist 		= LITTLE_TO_CPU(0					);	//-d
	info_header->f_exist 		= LITTLE_TO_CPU(0					);	//-f
	info_header->f              = LITTLE_TO_CPU(0					);
	info_header->dp_exist       = LITTLE_TO_CPU(0					);
	info_header->p              = LITTLE_TO_CPU(0					);
	info_header->h_exist 		= LITTLE_TO_CPU(0					);	//-h
	info_header->i_exist 		= LITTLE_TO_CPU(0					);	//-i
	info_header->k_exist 		= LITTLE_TO_CPU(0					);	//-k
	info_header->compmode       = LITTLE_TO_CPU(0					);
	info_header->l_exist 		= LITTLE_TO_CPU(0					);	//-l
	info_header->outlfe         = LITTLE_TO_CPU(0					);
	info_header->m_exist 		= LITTLE_TO_CPU(0					);	//-m
	info_header->mr_exist		= LITTLE_TO_CPU(0					);	//-mr
	info_header->m              = LITTLE_TO_CPU(0					);
	info_header->n_exist 		= LITTLE_TO_CPU(0					);	//-n
	info_header->outnchans      = LITTLE_TO_CPU(0					);
	info_header->o_exist		= LITTLE_TO_CPU(0					);	//-o
	info_header->oc_exist       = LITTLE_TO_CPU(0					);
	info_header->op_exist       = LITTLE_TO_CPU(0					);
	info_header->od_exist       = LITTLE_TO_CPU(0					);
	info_header->p_exist 		= LITTLE_TO_CPU(0					);	//-p
	tt_pcmscale 				= 0.0		;
	temp = (((float)tt_pcmscale + 0.005)*100);
	info_header->pcmscale		= LITTLE_TO_CPU(temp);	//((x + 0.005) * 100)
	info_header->q_exist 		= LITTLE_TO_CPU(0					);	//-q
	info_header->r_exist 		= LITTLE_TO_CPU(0					);	//-r
	info_header->framestart     = LITTLE_TO_CPU(0					);
	info_header->frameend       = LITTLE_TO_CPU(0xffffffff			);
	info_header->s_exist 		= LITTLE_TO_CPU(0					);	//-s
	info_header->stereomode     = LITTLE_TO_CPU(0					);
	info_header->u_exist 		= LITTLE_TO_CPU(0					);	//-u
	info_header->dualmode		= LITTLE_TO_CPU(0					);
	info_header->v_exist 		= LITTLE_TO_CPU(0					);	//-v
	info_header->verbose 		= LITTLE_TO_CPU(0					);
	info_header->w_exist 		= LITTLE_TO_CPU(0					);	//-w
	info_header->upsample       = LITTLE_TO_CPU(0					);
	info_header->x_exist 		= LITTLE_TO_CPU(0					);	//-x
	tt_dynscalehigh				= 0.0;
	temp = ((float)tt_dynscalehigh + 0.005)*100;
	info_header->dynscalehigh	= LITTLE_TO_CPU(temp);	//((x + 0.005) * 100)
	info_header->y_exist 		= LITTLE_TO_CPU(0					);	//-y
	tt_dynscalelow				= 0.0;
	temp=((float)tt_dynscalelow + 0.005)*100;
	info_header->dynscalelow	= LITTLE_TO_CPU(temp);	//((x + 0.005) * 100);
	info_header->z_exist 		= LITTLE_TO_CPU(0					);	//-z
	info_header->zp_exist       = LITTLE_TO_CPU(0					);
	info_header->zd_exist       = LITTLE_TO_CPU(0					);

	info_header->num_exist 		= LITTLE_TO_CPU(0					);	//how much -0~7

	info_header->chanval[0]		= LITTLE_TO_CPU(0					);	//-0
	info_header->chanval[1]		= LITTLE_TO_CPU(0					);	//-1
	info_header->chanval[2]		= LITTLE_TO_CPU(0					);	//-2
	info_header->chanval[3]		= LITTLE_TO_CPU(0					);	//-3
	info_header->chanval[4]		= LITTLE_TO_CPU(0					);	//-4
	info_header->chanval[5]		= LITTLE_TO_CPU(0					);	//-5
	info_header->chanval[6]		= LITTLE_TO_CPU(0					);	//-6
	info_header->chanval[7]		= LITTLE_TO_CPU(0					);	//-7
	info_header->routing_index[0] = LITTLE_TO_CPU(0					);
	info_header->routing_index[1] = LITTLE_TO_CPU(0					);
	info_header->routing_index[2] = LITTLE_TO_CPU(0					);
	info_header->routing_index[3] = LITTLE_TO_CPU(0					);
	info_header->routing_index[4] = LITTLE_TO_CPU(0					);
	info_header->routing_index[5] = LITTLE_TO_CPU(0					);
	info_header->routing_index[6] = LITTLE_TO_CPU(0					);
	info_header->routing_index[7] = LITTLE_TO_CPU(0					);
	info_header->byterev = LITTLE_TO_CPU(1					);
	//info_header->ddoutbaseaddr  = LITTLE_TO_CPU(ddout_start_addr				);
	//info_header->ddoutlength    = LITTLE_TO_CPU((ddout_end_addr+1-ddout_start_addr));
	//info_header->ddoutid        = LITTLE_TO_CPU(AUDIO_DDOUT_BUFF_ID					);
    //while(aa[read_cnt]!= 0xa)
    do{
    	//fread(&bb,1,1,cmd_file);

    	bb=aa[read_cnt];
    	read_cnt++;
    	if(bb=='-')
    	{
    		//fread(&bb,1,1,cmd_file);
    		bb=aa[read_cnt];
    		read_cnt++;
    		switch (bb)
    		{
    			case 'b':
				case 'B':
					info_header->b_exist 		= LITTLE_TO_CPU(1					);
					info_header->pcmwordtype	= LITTLE_TO_CPU(aa2i(&aa[read_cnt])		);
					break;
    			case 'c':
				case 'C':
					info_header->c_exist 		= LITTLE_TO_CPU(1					);
					info_header->kmode          = LITTLE_TO_CPU(aa2i(&aa[read_cnt])			);
    				break;
    			case 'd':
				case 'D':
					info_header->d_exist 		= LITTLE_TO_CPU(1					);	//-d
					//fread(&bb,1,1,cmd_file);
    				bb=aa[read_cnt];
    				read_cnt++;
					switch (bb)
					{
						case 'f':
						case 'F':
							info_header->f_exist 		= LITTLE_TO_CPU(1					);
							//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
							info_header->f              = LITTLE_TO_CPU(aa2i(&aa[read_cnt])					);
							break;
						case 'p':
						case 'P':
							info_header->dp_exist       = LITTLE_TO_CPU(1					);
							//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
							info_header->p              = LITTLE_TO_CPU(aa2i(&aa[read_cnt])				);
							break;
					}
					break;

				case 'h':
				case 'H':
					info_header->h_exist 		= LITTLE_TO_CPU(1					);	//-h
					break;
				case 'i':
				case 'I':
					info_header->i_exist 		= LITTLE_TO_CPU(1					);	//-i
					break;
				case 'k':
				case 'K':
					info_header->k_exist 		= LITTLE_TO_CPU(1					);	//-k
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
					info_header->compmode       = LITTLE_TO_CPU(aa2i(&aa[read_cnt])						);
					break;
				case 'l':
				case 'L':
					info_header->l_exist 		= LITTLE_TO_CPU(1					);	//-l
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
					info_header->outlfe       = LITTLE_TO_CPU(aa2i(&aa[read_cnt])					);
					break;
				case 'm':
				case 'M':
					info_header->m_exist 		= LITTLE_TO_CPU(1					);	//-m
					//fread(&bb,1,1,cmd_file);
    		bb=aa[read_cnt];

					switch (bb)
					{
						case 'r':
						case 'R':
							info_header->mr_exist		= LITTLE_TO_CPU(1					);	//-mr
							break;
						default:
							info_header->m              = LITTLE_TO_CPU(aa2i(&aa[read_cnt])						);
							break;
					}
					read_cnt++;
					break;
				case 'n':
				case 'N':
					info_header->n_exist 		= LITTLE_TO_CPU(1					);	//-n
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
					info_header->outnchans      = LITTLE_TO_CPU(aa2i(&aa[read_cnt])					);
					break;
				case 'o':
				case 'O':
					info_header->o_exist		= LITTLE_TO_CPU(1					);	//-o
					//fread(&bb,1,1,cmd_file);
    		bb=aa[read_cnt];
    		read_cnt++;
					switch (bb)
					{
                        case 'c'://dec71
                        case 'C':
							info_header->oc_exist       = LITTLE_TO_CPU(1					);
							break;
						case 'm'://dec51,dcv
                        case 'M':
							info_header->oc_exist       = LITTLE_TO_CPU(1					);
							break;
						case 'p':
						case 'P':
							info_header->op_exist       = LITTLE_TO_CPU(1					);
							break;
						case 'd':
						case 'D':
							info_header->od_exist       = LITTLE_TO_CPU(1					);
							//info_header->ddoutbaseaddr  = LITTLE_TO_CPU(ddout_start_addr				);
							//info_header->ddoutlength    = LITTLE_TO_CPU((ddout_end_addr+1-ddout_start_addr));
							//info_header->ddoutid        = LITTLE_TO_CPU(AUDIO_DDOUT_BUFF_ID					);


							break;
					}
					break;
				case 'p':
				case 'P':
					info_header->p_exist 		= LITTLE_TO_CPU(1					);	//-p
					//fread(&bb,1,1,cmd_file);
		    		bb=aa[read_cnt];
		    		read_cnt++;
					tt_pcmscale = atof(&bb);
					temp = (((float)tt_pcmscale + 0.005)*100);
					info_header->pcmscale		= LITTLE_TO_CPU(temp);	//((x + 0.005) * 100)
					break;
				case 'q':
				case 'Q':
					info_header->q_exist 		= LITTLE_TO_CPU(1					);	//-q
					break;
				case 'r':
				case 'R':
					info_header->r_exist 		= LITTLE_TO_CPU(1					);	//-r
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;

					info_header->framestart     = LITTLE_TO_CPU(aa2i(&aa[read_cnt])						);
					while((aa[read_cnt]!='-')&&(aa[read_cnt]!=' ')&&(aa[read_cnt]!='	')&&(aa[read_cnt]!=':'))
			{


				read_cnt++;

			}
					//fread(&bb,1,1,cmd_file);
    		bb=aa[read_cnt];
    		//read_cnt++;
					if(bb==':')
					{
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		read_cnt++;
					info_header->frameend     = LITTLE_TO_CPU(aa2i(&aa[read_cnt])					);


					}
					else
					{
					info_header->frameend       = LITTLE_TO_CPU(0xffffffff			);

					}
					break;
				case 's':
				case 'S':
					info_header->s_exist 		= LITTLE_TO_CPU(1					);	//-s
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
					info_header->stereomode     = LITTLE_TO_CPU(aa2i(&aa[read_cnt])						);
					break;

				case 'u':
				case 'U':
					info_header->u_exist 		= LITTLE_TO_CPU(1					);	//-u
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
					info_header->dualmode    = LITTLE_TO_CPU(aa2i(&aa[read_cnt])						);
					break;
				case 'v':
				case 'V':
					info_header->v_exist 		= LITTLE_TO_CPU(1					);	//-v
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
					info_header->verbose    = LITTLE_TO_CPU(aa2i(&aa[read_cnt])						);
					break;
				case 'w':
				case 'W':
					info_header->w_exist 		= LITTLE_TO_CPU(1					);	//-w
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
					info_header->upsample       = LITTLE_TO_CPU(aa2i(&aa[read_cnt])						);
					break;
				case 'x':
				case 'X':
					info_header->x_exist 		= LITTLE_TO_CPU(1					);	//-x
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
					tt_dynscalehigh				= aa2f(&aa[read_cnt])	;
					temp = ((float)tt_dynscalehigh + 0.005)*100;
					info_header->dynscalehigh	= LITTLE_TO_CPU(temp);	//((x + 0.005) * 100)
					break;
				case 'y':
				case 'Y':
					info_header->y_exist 		= LITTLE_TO_CPU(1					);	//-y
					//fread(&bb,1,1,cmd_file);
    		//bb=aa[read_cnt];
    		//read_cnt++;
					tt_dynscalelow				=  aa2f(&aa[read_cnt]);
					temp = ((float)tt_dynscalelow + 0.005)*100;
					info_header->dynscalelow	= LITTLE_TO_CPU(temp);	//((x + 0.005) * 100);
					break;
				case 'z':
				case 'Z':
					info_header->z_exist 		= LITTLE_TO_CPU(1					);	//-z
					//fread(&bb,1,1,cmd_file);
    		bb=aa[read_cnt];
    		read_cnt++;
					switch (bb)
					{
						case 'p':
						case 'P':
							info_header->zp_exist       = LITTLE_TO_CPU(1					);
							break;
						case 'd':
						case 'D':
							info_header->zd_exist       = LITTLE_TO_CPU(1					);
							break;
					}
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					//num_exist++;
					//info_header->num_exist 		= LITTLE_TO_CPU(num_exist					);
					chanval = bb - '0';
					//fread(&bb,1,1,cmd_file);
    		bb=aa[read_cnt];
    		read_cnt++;

					switch (bb)
					{
						case 'L':
                            routing_index = 0;
							break;
						case 'C':
                            routing_index = 1;
							break;
						case 'R':
                            routing_index = 2;
							break;
						case 'l':
                            routing_index = 3;
							break;
						case 'r':
                            routing_index = 4;
							break;
						case 's':
                            routing_index = 5;
							break;
						case 'x':
							//fread(&bb,1,1,cmd_file);
    		bb=aa[read_cnt];
    		read_cnt++;
							if (bb == '1')
							{
                                routing_index = 6;
							}
							else if (bb == '2')
							{
                                routing_index = 7;
							}

							break;

					}
					info_header->chanval[chanval]		= LITTLE_TO_CPU(1					);	//-0


					info_header->routing_index[chanval] = LITTLE_TO_CPU(routing_index					);


					break;

    		}

    	}
    }while(read_cnt<strlen(aa));

}

static int dobly_parse_init(GxDemuxer* demuxer)
{
	int i = 0;
	char* filename;
	char root[32] = {'\0'};
	char* tmpname = demuxer->stream->url;
	DemuxDolbyPriv* priv = (DemuxDolbyPriv *) demuxer->priv;

	while(tmpname)
	{
		filename = (++tmpname);
		tmpname = strstr(filename,"/");
		if(i++ == 1)
			memcpy(root,demuxer->stream->url,(int)(tmpname-demuxer->stream->url));
	}

#if CONFIG_INSIDE
	for(i=0;i<sizeof(cc)/256;i++)
	{
		if(strstr((cc[i]),filename))
			break;
	}
#else
	i = 0;
	{
		FILE* fpc;
		strncat(root,"/dolby.txt", strlen("/dolby.txt"));
		if((fpc = fopen(root,"r")) != NULL)
		{
#if 0
			while(1)
			{
				if(fgets(cc[i],512,fpc) == NULL)
				{
					fclose(fpc);
					gxlogd("dolby config read error 2\n");
					return -1;
				}
				else
				{
					gxlogd("%s\n",cc[1]);
					if(strstr((cc[i]),filename))
					{
						fclose(fpc);
						break;
					}
				}
			}
#else
			fgets(cc[i],512,fpc);
#endif
		}
		else
		{
			gxlogd("dolby config open error \n");
			return -1;
		}

		fclose(fpc);
	}
#endif

	if(i < sizeof(cc)/256)
	{
		char fc;
		dolby_parse(cc[i],(dolby_info_header*)(priv->header));
		if(strstr(cc[i],"dcv.exe"))
			priv->format = PLAYER_ACODEC_DBCV;
		else if(strstr(cc[i],"dec51.exe"))
			priv->format = PLAYER_ACODEC_DB51;
		else if(strstr(cc[i],"dec71.exe"))
			priv->format = PLAYER_ACODEC_DB71;

		fc = GxStream_ReadChar(demuxer->stream);

		((dolby_info_header*)(priv->header))->byterev = ((fc==0x77) ? 0x01000000:0);

        priv->big_endian = ((fc==0x77) ? 0:1);

		GxStream_Seek(demuxer->stream,0);

		return 0;
	}

	return -1;
}

static void demux_dolby_close(GxDemuxer* demuxer)
{
	DemuxDolbyPriv* priv = (DemuxDolbyPriv *) demuxer->priv;

	if(!priv)
		return;

	av_free(demuxer->priv);

	return;
}

static int demux_dolby_check_file(GxDemuxer* demuxer)
{
	int ret;
	DemuxDolbyPriv* priv;

	priv = av_mallocz(sizeof(DemuxDolbyPriv));

	if(!priv)
		return GX_DEMUXER_TYPE_UNKNOWN;

	demuxer->priv = priv;

	ret = dobly_parse_init(demuxer);

	if(ret < 0)
	{
		av_free(priv);
		gxlogd("demux_dolby_probe, failed to detect an Dolby stream\n");
		return GX_DEMUXER_TYPE_UNKNOWN;
	}

	return GX_DEMUXER_TYPE_DOLBY;

}

static GxDemuxer* demux_dolby_open(GxDemuxer* demuxer)
{
	GxStreamAudioHeader* sh;
	DemuxDolbyPriv* priv = demuxer->priv;

	sh = GxStreamHeader_AudioNew(demuxer,0, 0);
	sh->ds = demuxer->audio;
	sh->format = priv->format;
	sh->big_endian = priv->big_endian;

	sh->priv.para.data = av_mallocz(sizeof(priv->header));
	sh->priv.para.len = sizeof(priv->header);
	memcpy(sh->priv.para.data,priv->header,sh->priv.para.len);

	demuxer->audio->id = 0;
	demuxer->audio->sh = sh;
	sh->wf = av_mallocz(sizeof(WAVEFORMATEX));

    demuxer->duration = 100000;

	demuxer->filepos = GxStream_Tell(demuxer->stream);

	demuxer->stream->eof = 0;

	return demuxer;
}

static int demux_dolby_fill_buffer(GxDemuxer* demuxer, GxDemuxStream *ds)
{
	DemuxDolbyPriv* priv = (DemuxDolbyPriv *) demuxer->priv;
	int len;
	GxDemuxPacket* dp;

	if(demuxer->stream->eof || (demuxer->movi_end && GxStream_Tell(demuxer->stream) >= demuxer->movi_end))
        	return GX_PLAYER_ERROR;

	len = GxStream_Read(demuxer->stream, (uint8_t *)(priv->buf), DOLBY_UNIT);
	if(len > 0)
	{
		dp = GxDemuxPacket_Create(demuxer, NULL, len);
		if(! dp)
		{
			gxlogf( "fill_buffer, NEW_ADD_PACKET(%d)FAILED\n", len);
			return GX_PLAYER_ERROR;
		}

		GxDemuxPacket_Write(dp, (uint8_t *)(priv->buf), len);

		GxDemuxStream_AddPacket(demuxer->audio, dp);

		return GX_PLAYER_OK;
	}

	demuxer->audio->eof = 1;
	demuxer->stream->eof = 1;

	return GX_PLAYER_ERROR;
}


static void demux_dolby_seek(GxDemuxer* demuxer, int64_t  rel_seek_ms, int audio_delay, int flags)
{

}

static int demux_dolby_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	switch (cmd)
	{
		case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
			*((int64_t* )arg) =  demuxer->duration;
			return GX_DEMUXER_CTRL_OK;
		default:
			return GX_DEMUXER_CTRL_ERROR;
	}
}

GxDemuxerClass gx_demux_dolby = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer Dolby",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("demuxer","Dolby","No description","L.F","No comment"),

	.name        = "Demuxer Dolby",
	.type        = GX_DEMUXER_TYPE_DOLBY,
	.check_file  = demux_dolby_check_file,
	.open        = demux_dolby_open,
	.close       = demux_dolby_close,
	.fill_buffer = demux_dolby_fill_buffer,
	.seek        = demux_dolby_seek,
	.control     = demux_dolby_control,
};


