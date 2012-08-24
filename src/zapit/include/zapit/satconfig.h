/*
* satconfig.h
*/

#ifndef _SAT_CONFIG_H_
#define _SAT_CONFIG_H_

typedef struct sat_config {
	t_satellite_position position;
	int diseqc;
	int commited;
	int uncommited;
	int motor_position;
	int diseqc_order;
	int lnbOffsetLow;
	int lnbOffsetHigh;
	int lnbSwitch;
	int use_in_scan;
	int use_usals;
	std::string name;
	int have_channels;

	int feindex;	//to order channels 
	int type; 	//needed to dont rewrite services.xml by scan
} sat_config_t;

typedef enum diseqc_cmd_order {
	UNCOMMITED_FIRST,
	COMMITED_FIRST
} diseqc_cmd_order_t;

typedef std::map<t_satellite_position, sat_config_t> satellite_map_t;
typedef std::map<t_satellite_position, sat_config_t>::iterator sat_iterator_t;

typedef std::map <int, std::string> scan_list_t;
typedef std::map <int, std::string>::iterator scan_list_iterator_t;

extern satellite_map_t satellitePositions;					// defined in getServices.cpp
extern scan_list_t scanProviders;						// defined in zapit.cpp

/* cable map */
//typedef std::map<string, sat_config> cable_map_t;
//typedef std::map<string, sat_config_t>::iterator cable_iterator_t;

/* terrestrial map */
//typedef std::map<string, sat_config> terrestrial_map_t;
//typedef std::map<string, sat_config_t>::iterator terrestrial_iterator_t;


#endif		/* _SAT_CONFIG_H_ */
