/*
 * $Id: zapittypes.h,v 1.23 2004/02/24 23:50:57 thegoodguy Exp $
 *
 * zapit's types which are used by the clientlib - d-box2 linux project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
 * (C) 2002, 2003 by Andreas Oberritter <obi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __zapittypes_h__
#define __zapittypes_h__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <zapit/types.h>
#include <inttypes.h>
#include <string>
#include <map>

#include <linux/dvb/frontend.h>


/* diseqc types */
typedef enum {
	NO_DISEQC,
	MINI_DISEQC,
	SMATV_REMOTE_TUNING,
	DISEQC_1_0,
	DISEQC_1_1,
	DISEQC_1_2,
	DISEQC_ADVANCED
} diseqc_t;

/* dvb transmission types */
typedef enum {
	DVB_C,
	DVB_S,
	DVB_T
} delivery_system_t;

/* service types */
typedef enum {
	ST_RESERVED,
	ST_DIGITAL_TELEVISION_SERVICE,
	ST_DIGITAL_RADIO_SOUND_SERVICE,
	ST_TELETEXT_SERVICE,
	ST_NVOD_REFERENCE_SERVICE,
	ST_NVOD_TIME_SHIFTED_SERVICE,
	ST_MOSAIC_SERVICE,
	ST_PAL_CODED_SIGNAL,
	ST_SECAM_CODED_SIGNAL,
	ST_D_D2_MAC,
	ST_FM_RADIO,
	ST_NTSC_CODED_SIGNAL,
	ST_DATA_BROADCAST_SERVICE,
	ST_COMMON_INTERFACE_RESERVED,
	ST_RCS_MAP,
	ST_RCS_FLS,
	ST_DVB_MHP_SERVICE
} service_type_t;

/* complete transponder-parameters in a struct */
typedef struct TP_parameter
{
	uint64_t TP_id;					/* diseqc<<24 | feparams->frequency>>8 */
	uint8_t polarization;
	uint8_t diseqc;
	int scan_mode;
	
	struct dvb_frontend_parameters feparams;
} TP_params;

/* complete channel-parameters in a struct */
typedef struct Channel_parameter
{
	std::string name;
	t_service_id service_id;
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	unsigned char service_type;
	uint32_t TP_id;					/* diseqc<<24 | feparams->frequency>>8 */
} CH_params;

typedef struct TP_map
{
	TP_params TP;
	TP_map(const TP_params p_TP)
	{
		TP = p_TP;
	}
} t_transponder;

#define MAX_LNB 64 
typedef struct Zapit_config {
	int makeRemainingChannelsBouquet;
	int saveLastChannel;
	int scanSDT;
} t_zapit_config;

//nit_data: SatellitesPosition/feindex
struct nit_Data
{
	t_satellite_position satellitePosition;
	int feindex;
};

//complete zapit start thread-parameters in a struct
typedef struct ZAPIT_start_arg
{
	int lastchannelmode;
	t_channel_id startchanneltv_id;
	t_channel_id startchannelradio_id;
	int startchanneltv_nr;
	int startchannelradio_nr;
	int uselastchannel;
	
	int video_mode;
} Z_start_arg;
//

typedef enum {
	FE_SINGLE,
	//FE_TWIN,
	FE_LOOP,
	FE_NOTCONNECTED, // do we really need this
} fe_mode_t;

typedef std::map <uint32_t, TP_map> TP_map_t;
typedef std::map <uint32_t, TP_map>::iterator TP_iterator;

#define LIVE_DEMUX	0


#endif /* __zapittypes_h__ */
