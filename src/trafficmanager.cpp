// $Id$

/*
  Copyright (c) 2007-2015, Trustees of The Leland Stanford Junior University
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sstream>
#include <cmath>
#include <fstream>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <chrono>
#include <random>
#include <ctime>

#include "booksim.hpp"
#include "booksim_config.hpp"
#include "trafficmanager.hpp"
#include "batchtrafficmanager.hpp"
#include "random_utils.hpp"
#include "vc.hpp"
#include "packet_reply_info.hpp"
using namespace std;
TrafficManager * TrafficManager::New(Configuration const & config, vector<vector<int>>& LUT,
                                     vector<Network *> const & net)
{
    //cout<<LUT[0][0]<<"hhh"<<endl;
    int NODE_NUM = config.GetInt("core_x")*config.GetInt("core_y")*config.GetInt("chip_x")*config.GetInt("chip_y");


    TrafficManager * result = NULL;
    string sim_type = config.GetStr("sim_type");
    if((sim_type == "latency") || (sim_type == "throughput")) {
        result = new TrafficManager(config, LUT,net);
    } else if(sim_type == "batch") {
        result = new BatchTrafficManager(config, LUT,net);
    } else {
        cerr << "Unknown simulation type: " << sim_type << endl;
    }
    return result;
}

TrafficManager::TrafficManager( const Configuration &config,  vector<vector<int>>& LUT,const vector<Network *> & net )
    : Module( 0, "traffic_manager" ), _net(net), _empty_network(false),_deadlock_timer(0), _reset_time(0), _drain_time(-1), _cur_id(0), _cur_pid(0), total_phases(0),_time(0), run_time(0),packet_num(0),_LUT(LUT)
{
    group_phases.resize(files.size(),0);
    NODE_NUM = config.GetInt("core_x")*config.GetInt("core_y")*config.GetInt("chip_x")*config.GetInt("chip_y");

    flit.open("../watchfile/flit_latency");
	assert(flit.is_open());

    _node_num=config.GetInt("core_x")*config.GetInt("core_y")*config.GetInt("chip_x")*config.GetInt("chip_y");

    _nodes = _net[0]->NumNodes( );
    _routers = _net[0]->NumRouters( );

    _vcs = config.GetInt("num_vcs");
    _subnets = config.GetInt("subnets");

    _subnet.resize(Flit::NUM_FLIT_TYPES);
    _subnet[Flit::READ_REQUEST] = config.GetInt("read_request_subnet");
    _subnet[Flit::READ_REPLY] = config.GetInt("read_reply_subnet");
    _subnet[Flit::WRITE_REQUEST] = config.GetInt("write_request_subnet");
    _subnet[Flit::WRITE_REPLY] = config.GetInt("write_reply_subnet");

    // ============ Message priorities ============

    string priority = config.GetStr( "priority" );

    if ( priority == "class" ) {
        _pri_type = class_based;
    } else if ( priority == "age" ) {
        _pri_type = age_based;
    } else if ( priority == "network_age" ) {
        _pri_type = network_age_based;
    } else if ( priority == "local_age" ) {
        _pri_type = local_age_based;
    } else if ( priority == "queue_length" ) {
        _pri_type = queue_length_based;
    } else if ( priority == "hop_count" ) {
        _pri_type = hop_count_based;
    } else if ( priority == "sequence" ) {
        _pri_type = sequence_based;
    } else if ( priority == "none" ) {
        _pri_type = none;
    } else {
        Error( "Unkown priority value: " + priority );
    }

    // ============ Routing ============

    string rf = config.GetStr("routing_function") + "_" + config.GetStr("topology");
    map<string, tRoutingFunction>::const_iterator rf_iter = gRoutingFunctionMap.find(rf);
    if(rf_iter == gRoutingFunctionMap.end()) {
        Error("Invalid routing function: " + rf);
    }
    _rf = rf_iter->second;


    _lookahead_routing = !config.GetInt("routing_delay");
    _noq = config.GetInt("noq");
    if(_noq) {
        if(!_lookahead_routing) {
            Error("NOQ requires lookahead routing to be enabled.");
        }
    }

    // ============ Traffic ============

    _classes = config.GetInt("classes");
    phase_num=config.GetInt("total_phase");

    _use_read_write = config.GetIntArray("use_read_write");
    if(_use_read_write.empty()) {
        _use_read_write.push_back(config.GetInt("use_read_write"));
    }
    _use_read_write.resize(_classes, _use_read_write.back());

    _write_fraction = config.GetFloatArray("write_fraction");
    if(_write_fraction.empty()) {
        _write_fraction.push_back(config.GetFloat("write_fraction"));
    }
    _write_fraction.resize(_classes, _write_fraction.back());

    _read_request_size = config.GetIntArray("read_request_size");
    if(_read_request_size.empty()) {
        _read_request_size.push_back(config.GetInt("read_request_size"));
    }
    _read_request_size.resize(_classes, _read_request_size.back());

    _read_reply_size = config.GetIntArray("read_reply_size");
    if(_read_reply_size.empty()) {
        _read_reply_size.push_back(config.GetInt("read_reply_size"));
    }
    _read_reply_size.resize(_classes, _read_reply_size.back());

    _write_request_size = config.GetIntArray("write_request_size");
    if(_write_request_size.empty()) {
        _write_request_size.push_back(config.GetInt("write_request_size"));
    }
    _write_request_size.resize(_classes, _write_request_size.back());

    _write_reply_size = config.GetIntArray("write_reply_size");
    if(_write_reply_size.empty()) {
        _write_reply_size.push_back(config.GetInt("write_reply_size"));
    }
    _write_reply_size.resize(_classes, _write_reply_size.back());

    string packet_size_str = config.GetStr("packet_size");
    if(packet_size_str.empty()) {
        _packet_size.push_back(vector<int>(1, config.GetInt("packet_size")));
    } else {
        vector<string> packet_size_strings = tokenize_str(packet_size_str);
        for(size_t i = 0; i < packet_size_strings.size(); ++i) {
            _packet_size.push_back(tokenize_int(packet_size_strings[i]));
        }
    }
    _packet_size.resize(_classes, _packet_size.back());

    string packet_size_rate_str = config.GetStr("packet_size_rate");
    if(packet_size_rate_str.empty()) {
        int rate = config.GetInt("packet_size_rate");
        assert(rate >= 0);
        for(int c = 0; c < _classes; ++c) {
            int size = _packet_size[c].size();
            _packet_size_rate.push_back(vector<int>(size, rate));
            _packet_size_max_val.push_back(size * rate - 1);
        }
    } else {
        vector<string> packet_size_rate_strings = tokenize_str(packet_size_rate_str);
        packet_size_rate_strings.resize(_classes, packet_size_rate_strings.back());
        for(int c = 0; c < _classes; ++c) {
            vector<int> rates = tokenize_int(packet_size_rate_strings[c]);
            rates.resize(_packet_size[c].size(), rates.back());
            _packet_size_rate.push_back(rates);
            int size = rates.size();
            int max_val = -1;
            for(int i = 0; i < size; ++i) {
                int rate = rates[i];
                assert(rate >= 0);
                max_val += rate;
            }
            _packet_size_max_val.push_back(max_val);
        }
    }

    for(int c = 0; c < _classes; ++c) {
        if(_use_read_write[c]) {
            _packet_size[c] =
                vector<int>(1, (_read_request_size[c] + _read_reply_size[c] +
                                _write_request_size[c] + _write_reply_size[c]) / 2);
            _packet_size_rate[c] = vector<int>(1, 1);
            _packet_size_max_val[c] = 0;
        }
    }

    _load = config.GetFloatArray("injection_rate");
    if(_load.empty()) {
        _load.push_back(config.GetFloat("injection_rate"));
    }
    _load.resize(_classes, _load.back());

    if(config.GetInt("injection_rate_uses_flits")) {
        for(int c = 0; c < _classes; ++c)
            _load[c] /= _GetAveragePacketSize(c);
    }

    _traffic = config.GetStrArray("traffic");
    _traffic.resize(_classes, _traffic.back());

    _traffic_pattern.resize(_classes);

    _class_priority = config.GetIntArray("class_priority");
    if(_class_priority.empty()) {
        _class_priority.push_back(config.GetInt("class_priority"));
    }
    _class_priority.resize(_classes, _class_priority.back());

    vector<string> injection_process = config.GetStrArray("injection_process");
    injection_process.resize(_classes, injection_process.back());

    _injection_process.resize(_classes);


    for(int c = 0; c < _classes; ++c) {
        _traffic_pattern[c] = TrafficPattern::New(_traffic[c], _nodes, &config);
        _injection_process[c] = InjectionProcess::New(injection_process[c], _nodes, _load[c], LUT,&config);
    }

    // ============ Injection VC states  ============

    _buf_states.resize(_nodes);
    _last_vc.resize(_nodes);
    _last_class.resize(_nodes);

    for ( int source = 0; source < _nodes; ++source ) {
        _buf_states[source].resize(_subnets);
        _last_class[source].resize(_subnets, 0);
        _last_vc[source].resize(_subnets);
        for ( int subnet = 0; subnet < _subnets; ++subnet ) {
            ostringstream tmp_name;
            tmp_name << "terminal_buf_state_" << source << "_" << subnet;
            BufferState * bs = new BufferState( config, this, tmp_name.str( ) );
            int vc_alloc_delay = config.GetInt("vc_alloc_delay");
            int sw_alloc_delay = config.GetInt("sw_alloc_delay");
            int router_latency = config.GetInt("routing_delay") + (config.GetInt("speculative") ? max(vc_alloc_delay, sw_alloc_delay) : (vc_alloc_delay + sw_alloc_delay));
            int min_latency = 1 + _net[subnet]->GetInject(source)->GetLatency() + router_latency + _net[subnet]->GetInjectCred(source)->GetLatency();
            bs->SetMinLatency(min_latency);
            _buf_states[source][subnet] = bs;
            _last_vc[source][subnet].resize(_classes, -1);
        }
    }

#ifdef TRACK_FLOWS
    _outstanding_credits.resize(_classes);
    for(int c = 0; c < _classes; ++c) {
        _outstanding_credits[c].resize(_subnets, vector<int>(_nodes, 0));
    }
    _outstanding_classes.resize(_nodes);
    for(int n = 0; n < _nodes; ++n) {
        _outstanding_classes[n].resize(_subnets, vector<queue<int> >(_vcs));
    }
#endif

    // ============ Injection queues ============

    _total_phase = config.GetInt( "total_phase" );
    _qtime.resize(_nodes);
    _qdrained.resize(_nodes);
    _partial_packets.resize(_nodes);


    for ( int s = 0; s < _nodes; ++s ) {
        _qtime[s].resize(_classes);
        _qdrained[s].resize(_classes);
        _partial_packets[s].resize(_classes);
    }


    _total_in_flight_flits.resize(_classes);
    _measured_in_flight_flits.resize(_classes);
    _retired_packets.resize(_classes);

    _packet_seq_no.resize(_nodes);
    _repliesPending.resize(_nodes);
    _requestsOutstanding.resize(_nodes);

    _hold_switch_for_packet = config.GetInt("hold_switch_for_packet");

    // ============ Simulation parameters ============

    _total_sims = config.GetInt( "sim_count" );

    _router.resize(_subnets);
    for (int i=0; i < _subnets; ++i) {
        _router[i] = _net[i]->GetRouters();
    }

    //seed the network
    int seed;
    if(config.GetStr("seed") == "time") {
      seed = int(time(NULL));
      cout << "SEED: seed=" << seed << endl;
    } else {
      seed = config.GetInt("seed");
    }
    RandomSeed(seed);

    _measure_latency = (config.GetStr("sim_type") == "latency");

    _sample_period = config.GetInt( "sample_period" );

    _phase_period = config.GetIntArray( "phase_period" );
    _max_samples    = config.GetInt( "max_samples" );
    _warmup_periods = config.GetInt( "warmup_periods" );

    _measure_stats = config.GetIntArray( "measure_stats" );
    if(_measure_stats.empty()) {
        _measure_stats.push_back(config.GetInt("measure_stats"));
    }
    _measure_stats.resize(_classes, _measure_stats.back());
    _pair_stats = (config.GetInt("pair_stats")==1);

    _latency_thres = config.GetFloatArray( "latency_thres" );
    if(_latency_thres.empty()) {
        _latency_thres.push_back(config.GetFloat("latency_thres"));
    }
    _latency_thres.resize(_classes, _latency_thres.back());

    _warmup_threshold = config.GetFloatArray( "warmup_thres" );
    if(_warmup_threshold.empty()) {
        _warmup_threshold.push_back(config.GetFloat("warmup_thres"));
    }
    _warmup_threshold.resize(_classes, _warmup_threshold.back());

    _acc_warmup_threshold = config.GetFloatArray( "acc_warmup_thres" );
    if(_acc_warmup_threshold.empty()) {
        _acc_warmup_threshold.push_back(config.GetFloat("acc_warmup_thres"));
    }
    _acc_warmup_threshold.resize(_classes, _acc_warmup_threshold.back());

    _stopping_threshold = config.GetFloatArray( "stopping_thres" );
    if(_stopping_threshold.empty()) {
        _stopping_threshold.push_back(config.GetFloat("stopping_thres"));
    }
    _stopping_threshold.resize(_classes, _stopping_threshold.back());

    _acc_stopping_threshold = config.GetFloatArray( "acc_stopping_thres" );
    if(_acc_stopping_threshold.empty()) {
        _acc_stopping_threshold.push_back(config.GetFloat("acc_stopping_thres"));
    }
    _acc_stopping_threshold.resize(_classes, _acc_stopping_threshold.back());

    _include_queuing = config.GetInt( "include_queuing" );

    _print_csv_results = config.GetInt( "print_csv_results" );
    _deadlock_warn_timeout = config.GetInt( "deadlock_warn_timeout" );

    string watch_file = config.GetStr( "watch_file" );
    if((watch_file != "") && (watch_file != "-")) {
        _LoadWatchList(watch_file);
    }

    vector<int> watch_flits = config.GetIntArray("watch_flits");
    for(size_t i = 0; i < watch_flits.size(); ++i) {
        _flits_to_watch.insert(watch_flits[i]);
    }

    vector<int> watch_packets = config.GetIntArray("watch_packets");
    for(size_t i = 0; i < watch_packets.size(); ++i) {
        _packets_to_watch.insert(watch_packets[i]);
    }

    string stats_out_file = config.GetStr( "stats_out" );
    if(stats_out_file == "") {
        _stats_out = NULL;
    } else if(stats_out_file == "-") {
        _stats_out = &cout;
    } else {
        _stats_out = new ofstream(stats_out_file.c_str());
        config.WriteMatlabFile(_stats_out);
    }

#ifdef TRACK_FLOWS
    _injected_flits.resize(_classes, vector<int>(_nodes, 0));
    _ejected_flits.resize(_classes, vector<int>(_nodes, 0));
    string injected_flits_out_file = config.GetStr( "injected_flits_out" );
    if(injected_flits_out_file == "") {
        _injected_flits_out = NULL;
    } else {
        _injected_flits_out = new ofstream(injected_flits_out_file.c_str());
    }
    string received_flits_out_file = config.GetStr( "received_flits_out" );
    if(received_flits_out_file == "") {
        _received_flits_out = NULL;
    } else {
        _received_flits_out = new ofstream(received_flits_out_file.c_str());
    }
    string stored_flits_out_file = config.GetStr( "stored_flits_out" );
    if(stored_flits_out_file == "") {
        _stored_flits_out = NULL;
    } else {
        _stored_flits_out = new ofstream(stored_flits_out_file.c_str());
    }
    string sent_flits_out_file = config.GetStr( "sent_flits_out" );
    if(sent_flits_out_file == "") {
        _sent_flits_out = NULL;
    } else {
        _sent_flits_out = new ofstream(sent_flits_out_file.c_str());
    }
    string outstanding_credits_out_file = config.GetStr( "outstanding_credits_out" );
    if(outstanding_credits_out_file == "") {
        _outstanding_credits_out = NULL;
    } else {
        _outstanding_credits_out = new ofstream(outstanding_credits_out_file.c_str());
    }
    string ejected_flits_out_file = config.GetStr( "ejected_flits_out" );
    if(ejected_flits_out_file == "") {
        _ejected_flits_out = NULL;
    } else {
        _ejected_flits_out = new ofstream(ejected_flits_out_file.c_str());
    }
    string active_packets_out_file = config.GetStr( "active_packets_out" );
    if(active_packets_out_file == "") {
        _active_packets_out = NULL;
    } else {
        _active_packets_out = new ofstream(active_packets_out_file.c_str());
    }
#endif

#ifdef TRACK_CREDITS
    string used_credits_out_file = config.GetStr( "used_credits_out" );
    if(used_credits_out_file == "") {
        _used_credits_out = NULL;
    } else {
        _used_credits_out = new ofstream(used_credits_out_file.c_str());
    }
    string free_credits_out_file = config.GetStr( "free_credits_out" );
    if(free_credits_out_file == "") {
        _free_credits_out = NULL;
    } else {
        _free_credits_out = new ofstream(free_credits_out_file.c_str());
    }
    string max_credits_out_file = config.GetStr( "max_credits_out" );
    if(max_credits_out_file == "") {
        _max_credits_out = NULL;
    } else {
        _max_credits_out = new ofstream(max_credits_out_file.c_str());
    }
#endif

    // ============ Statistics ============

    _plat_stats.resize(_classes);
    _overall_min_plat.resize(_classes, 0.0);
    _overall_avg_plat.resize(_classes, 0.0);
    _overall_max_plat.resize(_classes, 0.0);

    _nlat_stats.resize(_classes);
    _overall_min_nlat.resize(_classes, 0.0);
    _overall_avg_nlat.resize(_classes, 0.0);
    _overall_max_nlat.resize(_classes, 0.0);

    _flat_stats.resize(_classes);
    _overall_min_flat.resize(_classes, 0.0);
    _overall_avg_flat.resize(_classes, 0.0);
    _overall_max_flat.resize(_classes, 0.0);

    _frag_stats.resize(_classes);
    _overall_min_frag.resize(_classes, 0.0);
    _overall_avg_frag.resize(_classes, 0.0);
    _overall_max_frag.resize(_classes, 0.0);

    _flat_phase.resize(phase_num);

    if(_pair_stats){
        _pair_plat.resize(_classes);
        _pair_nlat.resize(_classes);
        _pair_flat.resize(_classes);
    }

    _hop_stats.resize(_classes);
    _overall_hop_stats.resize(_classes, 0.0);

    _sent_packets.resize(_classes);
    _overall_min_sent_packets.resize(_classes, 0.0);
    _overall_avg_sent_packets.resize(_classes, 0.0);
    _overall_max_sent_packets.resize(_classes, 0.0);
    _accepted_packets.resize(_classes);
    _overall_min_accepted_packets.resize(_classes, 0.0);
    _overall_avg_accepted_packets.resize(_classes, 0.0);
    _overall_max_accepted_packets.resize(_classes, 0.0);

    _sent_flits.resize(_classes);
    _overall_min_sent.resize(_classes, 0.0);
    _overall_avg_sent.resize(_classes, 0.0);
    _overall_max_sent.resize(_classes, 0.0);
    _accepted_flits.resize(_classes);
    _overall_min_accepted.resize(_classes, 0.0);
    _overall_avg_accepted.resize(_classes, 0.0);
    _overall_max_accepted.resize(_classes, 0.0);

#ifdef TRACK_STALLS
    _buffer_busy_stalls.resize(_classes);
    _buffer_conflict_stalls.resize(_classes);
    _buffer_full_stalls.resize(_classes);
    _buffer_reserved_stalls.resize(_classes);
    _crossbar_conflict_stalls.resize(_classes);
    _overall_buffer_busy_stalls.resize(_classes, 0);
    _overall_buffer_conflict_stalls.resize(_classes, 0);
    _overall_buffer_full_stalls.resize(_classes, 0);
    _overall_buffer_reserved_stalls.resize(_classes, 0);
    _overall_crossbar_conflict_stalls.resize(_classes, 0);
#endif
    for ( int c = 0; c < phase_num; ++c ) {
        ostringstream tmp_name;

        tmp_name << "flat_phase_" << c;
        _flat_phase[c] = new Stats( this, tmp_name.str( ), 1.0, 1000 );
        tmp_name.str("");
    }
    for ( int c = 0; c < _classes; ++c ) {
        ostringstream tmp_name;

        tmp_name << "plat_stat_" << c;
        _plat_stats[c] = new Stats( this, tmp_name.str( ), 1.0, 1000 );
        _stats[tmp_name.str()] = _plat_stats[c];
        tmp_name.str("");

        tmp_name << "nlat_stat_" << c;
        _nlat_stats[c] = new Stats( this, tmp_name.str( ), 1.0, 1000 );
        _stats[tmp_name.str()] = _nlat_stats[c];
        tmp_name.str("");

        tmp_name << "flat_stat_" << c;
        _flat_stats[c] = new Stats( this, tmp_name.str( ), 1.0, 1000 );
        _stats[tmp_name.str()] = _flat_stats[c];
        tmp_name.str("");

        tmp_name << "frag_stat_" << c;
        _frag_stats[c] = new Stats( this, tmp_name.str( ), 1.0, 100 );
        _stats[tmp_name.str()] = _frag_stats[c];
        tmp_name.str("");

        tmp_name << "hop_stat_" << c;
        _hop_stats[c] = new Stats( this, tmp_name.str( ), 1.0, 20 );
        _stats[tmp_name.str()] = _hop_stats[c];
        tmp_name.str("");

        if(_pair_stats){
            _pair_plat[c].resize(_nodes*_nodes);
            _pair_nlat[c].resize(_nodes*_nodes);
            _pair_flat[c].resize(_nodes*_nodes);
        }

        _sent_packets[c].resize(_nodes, 0);
        _accepted_packets[c].resize(_nodes, 0);
        _sent_flits[c].resize(_nodes, 0);
        _accepted_flits[c].resize(_nodes, 0);

#ifdef TRACK_STALLS
        _buffer_busy_stalls[c].resize(_subnets*_routers, 0);
        _buffer_conflict_stalls[c].resize(_subnets*_routers, 0);
        _buffer_full_stalls[c].resize(_subnets*_routers, 0);
        _buffer_reserved_stalls[c].resize(_subnets*_routers, 0);
        _crossbar_conflict_stalls[c].resize(_subnets*_routers, 0);
#endif
        if(_pair_stats){
            for ( int i = 0; i < _nodes; ++i ) {
                for ( int j = 0; j < _nodes; ++j ) {
                    tmp_name << "pair_plat_stat_" << c << "_" << i << "_" << j;
                    _pair_plat[c][i*_nodes+j] = new Stats( this, tmp_name.str( ), 1.0, 250 );
                    _stats[tmp_name.str()] = _pair_plat[c][i*_nodes+j];
                    tmp_name.str("");

                    tmp_name << "pair_nlat_stat_" << c << "_" << i << "_" << j;
                    _pair_nlat[c][i*_nodes+j] = new Stats( this, tmp_name.str( ), 1.0, 250 );
                    _stats[tmp_name.str()] = _pair_nlat[c][i*_nodes+j];
                    tmp_name.str("");

                    tmp_name << "pair_flat_stat_" << c << "_" << i << "_" << j;
                    _pair_flat[c][i*_nodes+j] = new Stats( this, tmp_name.str( ), 1.0, 250 );
                    _stats[tmp_name.str()] = _pair_flat[c][i*_nodes+j];
                    tmp_name.str("");
                }
            }
        }
    }

    _slowest_flit.resize(_classes, -1);
    _slowest_packet.resize(_classes, -1);
    _slowest_flit_phase.resize(3,vector<int>(phase_num, -1));



}

TrafficManager::~TrafficManager( )
{

    for ( int source = 0; source < _nodes; ++source ) {
        for ( int subnet = 0; subnet < _subnets; ++subnet ) {
            delete _buf_states[source][subnet];
        }
    }
    for ( int c = 0; c < phase_num; ++c ) {
        delete _flat_phase[c];
    }
    for ( int c = 0; c < _classes; ++c ) {
        delete _plat_stats[c];
        delete _nlat_stats[c];
        delete _flat_stats[c];
        delete _frag_stats[c];
        delete _hop_stats[c];

        delete _traffic_pattern[c];
        delete _injection_process[c];
        if(_pair_stats){
            for ( int i = 0; i < _nodes; ++i ) {
                for ( int j = 0; j < _nodes; ++j ) {
                    delete _pair_plat[c][i*_nodes+j];
                    delete _pair_nlat[c][i*_nodes+j];
                    delete _pair_flat[c][i*_nodes+j];
                }
            }
        }
    }

    if(gWatchOut && (gWatchOut != &cout)) delete gWatchOut;
    if(_stats_out && (_stats_out != &cout)) delete _stats_out;

#ifdef TRACK_FLOWS
    if(_injected_flits_out) delete _injected_flits_out;
    if(_received_flits_out) delete _received_flits_out;
    if(_stored_flits_out) delete _stored_flits_out;
    if(_sent_flits_out) delete _sent_flits_out;
    if(_outstanding_credits_out) delete _outstanding_credits_out;
    if(_ejected_flits_out) delete _ejected_flits_out;
    if(_active_packets_out) delete _active_packets_out;
#endif

#ifdef TRACK_CREDITS
    if(_used_credits_out) delete _used_credits_out;
    if(_free_credits_out) delete _free_credits_out;
    if(_max_credits_out) delete _max_credits_out;
#endif

    PacketReplyInfo::FreeAll();
    Flit::FreeAll();
    Credit::FreeAll();
}

int TrafficManager::findgroup(int src){
        map<int ,int > ::iterator current_group;
        current_group = phase_group.find(src);
        return current_group->second;
}

void TrafficManager::_RetireFlit( Flit *f, int dest )
{
    _deadlock_timer = 0;
  //  cout<<f->id<<endl;

    assert(_total_in_flight_flits[f->cl].count(f->id) > 0);
    _total_in_flight_flits[f->cl].erase(f->id);

    if(f->record) {
        assert(_measured_in_flight_flits[f->cl].count(f->id) > 0);
        _measured_in_flight_flits[f->cl].erase(f->id);
    }

    if ( f->watch ) {
        *gWatchOut << GetSimTime() << " | "
                   << "node" << dest << " | "
                   << "Retiring flit " << f->id
                   << " (packet " << f->pid
                   << ", src = " << f->src
                   << ", dest = " << f->dest
                   << ", hops = " << f->hops
                   << ", flat = " << f->atime - f->itime
                   << ")." << endl;
    }
    if(f->phase>-1&&f->multi==false)
    {phase_cost[f->phase]+=f->hops;
    phase_all_packet_num[f->phase][f->dest]--;}
    // cout<<"retire flit "<<f->id<<" src= "<<f->src<<endl;






    if ( f->head && ( f->dest != dest ) ) {
        ostringstream err;
        err << "Flit " << f->id << " arrived at incorrect output " << dest;
        Error( err.str( ) );
    }
    //if(f->src==1016&&f->dest==989)cout<<GetSimTime()<<"|"<<f->atime - f->itime<<endl;

    if((_slowest_flit[f->cl] < 0) ||
       (_flat_stats[f->cl]->Max() < (f->atime - f->itime)))
        _slowest_flit[f->cl] = f->id;
if(f->phase>-1){
    if((_slowest_flit_phase[0][f->phase] < 0) ||
       (_flat_phase[f->phase]->Max() < (f->atime - f->itime)))
        {
            _slowest_flit_phase[0][f->phase] = f->id;
            _slowest_flit_phase[1][f->phase] = f->src;
            _slowest_flit_phase[2][f->phase] = f->dest;
        }
    _flat_phase[f->phase]->AddSample( f->atime - f->itime,f->id);
    }
    _flat_stats[f->cl]->AddSample( f->atime - f->itime,f->id);
   // flit<<(f->atime - f->itime)<<endl;
    if(_pair_stats){
        _pair_flat[f->cl][f->src*_nodes+dest]->AddSample( f->atime - f->itime,f->id );
    }

    if ( f->tail ) {
        Flit * head;
        if(f->head) {
            head = f;
        } else {
            map<int, Flit *>::iterator iter = _retired_packets[f->cl].find(f->pid);
            assert(iter != _retired_packets[f->cl].end());
            head = iter->second;
            _retired_packets[f->cl].erase(iter);
            assert(head->head);
            assert(f->pid == head->pid);
        }
        if ( f->watch ) {
            *gWatchOut << GetSimTime() << " | "
                       << "node" << dest << " | "
                       << "Retiring packet " << f->pid
                       << " (plat = " << f->atime - head->ctime
                       << ", nlat = " << f->atime - head->itime
                       << ", frag = " << (f->atime - head->atime) - (f->id - head->id) // NB: In the spirit of solving problems using ugly hacks, we compute the packet length by taking advantage of the fact that the IDs of flits within a packet are contiguous.
                       << ", src = " << head->src
                       << ", dest = " << head->dest
                       << ")." << endl;
        }


        //code the source of request, look carefully, its tricky ;)
        if (f->type == Flit::READ_REQUEST || f->type == Flit::WRITE_REQUEST) {
            PacketReplyInfo* rinfo = PacketReplyInfo::New();
            rinfo->source = f->src;
            rinfo->time = f->atime;
            rinfo->record = f->record;
            rinfo->type = f->type;
            _repliesPending[dest].push_back(rinfo);
        } else {
            if(f->type == Flit::READ_REPLY || f->type == Flit::WRITE_REPLY  ){
                _requestsOutstanding[dest]--;
            } else if(f->type == Flit::ANY_TYPE) {
                _requestsOutstanding[f->src]--;
            }

        }


        // Only record statistics once per packet (at tail)
        // and based on the simulation state
        if ( ( _sim_state == warming_up ) || f->record ) {

            _hop_stats[f->cl]->AddSample( f->hops ,f->id);

            if((_slowest_packet[f->cl] < 0) ||
               (_plat_stats[f->cl]->Max() < (f->atime - head->itime)))
                _slowest_packet[f->cl] = f->pid;
            _plat_stats[f->cl]->AddSample( f->atime - head->ctime,f->id);
            _nlat_stats[f->cl]->AddSample( f->atime - head->itime,f->id);
            _frag_stats[f->cl]->AddSample( (f->atime - head->atime) - (f->id - head->id) ,f->id);

            if(_pair_stats){
                _pair_plat[f->cl][f->src*_nodes+dest]->AddSample( f->atime - head->ctime ,f->id);
                _pair_nlat[f->cl][f->src*_nodes+dest]->AddSample( f->atime - head->itime ,f->id);
            }
        }

        if(f != head) {
            head->Free();
        }

    }

    if(f->head && !f->tail) {
        _retired_packets[f->cl].insert(make_pair(f->pid, f));
    } else {
        f->Free();
    }
}

int TrafficManager::_IssuePacket( int source, int cl )
{

    int result = 0;
   // cout<<_LUT[0][0]<<"hhh"<<endl;

//    if(phase_nodes_compute[group_phases[findgroup(source)]*2][source]>0&&compute_mode==0)
//    {
//     //    if(phase_nodes_compute[group_phases[findgroup(source)]*2][source]==0)
//     //    {cout<<"Node "<<source<<" starts to route in phase "<<group_phases[findgroup(source)]<<endl;}
//        return 0;
//    }



    if(_use_read_write[cl]){ //use read and write
        //check queue for waiting replies.
        //check to make sure it is on time yet
        if (!_repliesPending[source].empty()) {
            if(_repliesPending[source].front()->time <= _time) {
                result = -1;
            }
        } else {


            //produce a packet

            if(_injection_process[cl]->test(source,phase_packet_num,run_time,group_phases[findgroup(source)])) {


                //coin toss to determine request type.
                result = (RandomFloat() < _write_fraction[cl]) ? 2 : 1;

                _requestsOutstanding[source]++;
            }
        }
    } else { //normal mode

        result = _injection_process[cl]->test(source,phase_packet_num,run_time,group_phases[findgroup(source)]) ? 1 : 0;


        //cout<<LUT[0][0]<<"ghghghhg"<<endl;
        _requestsOutstanding[source]++;
    }
    if(result != 0) {
        _packet_seq_no[source]++;
    }
    return result;
}

void TrafficManager::_GeneratePacket( int source, int stype,
                                      int cl, int time )
{
    assert(stype!=0);

    Flit::FlitType packet_type = Flit::ANY_TYPE;
    int size = _GetNextPacketSize(cl); //input size
    int pid = _cur_pid++;
    assert(_cur_pid);

    int packet_destination = _traffic_pattern[cl]->dest(source,_LUT,run_time,group_phases[findgroup(source)]);

    if(stype==10){
        //if(packet_destination>-1&&packet_destination<10240){}
       // else{
            packet_destination = RandomInt(159);
            //(int(source/160)==int((source+16)/160))?source+16:source-16;
       // }

        //cout<<GetSimTime()<<"|"<<source<<"->"<<packet_destination<<endl;
    }
    bool record = false;
    bool watch = gWatchOut && (_packets_to_watch.count(pid) > 0);
    if(_use_read_write[cl]){
        if(stype > 0) {
            if (stype == 1||stype == 10) {
                packet_type = Flit::READ_REQUEST;
                size = _read_request_size[cl];//testbigsize
            } else if (stype == 2) {
                packet_type = Flit::WRITE_REQUEST;
                size = _write_request_size[cl];
            }
            else {
                ostringstream err;
                err << "Invalid packet type: " << packet_type;
                Error( err.str( ) );
            }
        } else {
            PacketReplyInfo* rinfo = _repliesPending[source].front();
            if (rinfo->type == Flit::READ_REQUEST) {//read reply
                size = _read_reply_size[cl];
                packet_type = Flit::READ_REPLY;
            } else if(rinfo->type == Flit::WRITE_REQUEST) {  //write reply
                size = _write_reply_size[cl];
                packet_type = Flit::WRITE_REPLY;
            } else {
                ostringstream err;
                err << "Invalid packet type: " << rinfo->type;
                Error( err.str( ) );
            }
            packet_destination = rinfo->source;
            time = rinfo->time;
            record = rinfo->record;
            _repliesPending[source].pop_front();
            rinfo->Free();
        }
    }

    if ((packet_destination <0) || (packet_destination >= _nodes)) {
        ostringstream err;
        err << "Incorrect packet destination " << packet_destination
            << " for stype " << packet_type;
        Error( err.str( ) );
    }

    if ( ( _sim_state == running ) ||
         ( ( _sim_state == draining ) && ( time < _drain_time ) ) ) {
        record = _measure_stats[cl];
        //cout<<"Record"<<record<<endl;
    }

    int subnetwork = ((packet_type == Flit::ANY_TYPE) ?
                      RandomInt(_subnets-1) :
                      _subnet[packet_type]);

    if ( watch ) {
        *gWatchOut << GetSimTime() << " | "
                   << "node" << source << " | "
                   << "Enqueuing packet " << pid
                   << " at time " << time
                   << "." << endl;
    }


    for ( int i = 0; i < size; ++i ) {
        Flit * f  = Flit::New();
        f->id     = _cur_id++;

        assert(_cur_id);
        f->pid    = pid;
        f->watch  = watch | (gWatchOut && (_flits_to_watch.count(f->id) > 0));
        f->subnetwork = subnetwork;
        f->src    = source;
    //    if(f->src==159)cout<<f->id <<" "<<packet_destination<<endl;

        f->phase  = group_phases[findgroup(source)];
        // cout<<f->phase<<endl;
        if(stype==10){f->phase  = -1;}
        f->ctime  = time;
        f->record = record;
        f->cl     = cl;
     //  f->multi  = (packet_mul[total_phases][source]>0);
        f->multi  = false;
        if(stype!=10)
        {phase_all_packet_num[f->phase][f->src]--;
        // cout<<f->src<<" "<<phase_all_packet_num[f->phase][f->src]<<endl;
        }//testbigsize
    //    cout<<"phase "<<f->phase<<" New Flit "<<f->id<<"src= "<<f->src<<endl;

        //  if(phase_all_packet_num[f->phase][f->src]==0){

        //     //cout<<"new phase"<<f->phase<<" node"<<f->dest<<" ends"<<endl;
        //     phase_end_nodes[f->phase][findgroup(f->src)]++;
        //     if(phase_end_nodes[f->phase][findgroup(f->src)]==group_size[f->phase][findgroup(f->src)])
        //     {
        //         if(group_phases[findgroup(f->src)]<_total_phase-1){group_phases[findgroup(f->src)]++;}
        //         cout<<"group "<<findgroup(f->dest)<<" enters phase "<<f->phase+1<<endl;
        //         }
        //     }



        _total_in_flight_flits[f->cl].insert(make_pair(f->id, f));
        if(record) {
          //  cout<<"re"<<endl;
            _measured_in_flight_flits[f->cl].insert(make_pair(f->id, f));
        }

        if(gTrace){
            cout<<"New Flit "<<f->src<<endl;
        }
        f->type = packet_type;

        if ( i == 0 ) { // Head flit
            f->head = true;
            //packets are only generated to nodes smaller or equal to limit
            f->dest = packet_destination;
        } else {
            f->head = false;
            f->dest = -1;
        }

       //if(f->src==632&&f->dest==968)cout<<f->id<<endl;
       if(stype!=10)
        {
            if(sample){
                // cout<<f->src<<" "<<f->dest<<endl;

                nodenum[f->phase][source][0]--;
                // cout<<source<<" "<<nodedes[f->phase][source]<<" "<<nodenum[f->phase][source][0]<<endl;
                if(nodenum[f->phase][source][0]==0)
                {
                vector<int>::iterator k = nodedes[f->phase][source].begin();
                if(nodedes[f->phase][source].size()>0)
                {nodedes[f->phase][source].erase(k);
                vector<int>::iterator kk = nodenum[f->phase][source].begin();
                nodenum[f->phase][source].erase(kk);}//删除第一个元素}

                //删除第一个元素



    }



            }
            else{

            map<int,int>::iterator current_dest;
            current_dest=LUT[f->phase][source].find(f->dest+1);
            if(current_dest!=LUT[f->phase][source].end())
            {
            LUT[f->phase][source][f->dest+1]--;
            //cout<<f->phase<<" "<<source<<" left "<<LUT[f->phase][source].size()<<endl;
            if(LUT[f->phase][source][f->dest+1]==0){LUT[f->phase][source].erase(current_dest);
            //cout<<f->phase<<" "<<source<<" left "<<LUT[f->phase][source].size()<<endl;
            }
        }
            }
}


    //    cout<<_time<<" "<<f->src<<"generate"<<f->id<<"to"<<f->dest<<endl;
     //if(run_time>1)   cout<<"New Flit "<<f->src<<"to"<<f->dest<<endl;
        switch( _pri_type ) {
        case class_based:
            f->pri = _class_priority[cl];
            assert(f->pri >= 0);
            break;
        case age_based:
            f->pri = numeric_limits<int>::max() - time;
            assert(f->pri >= 0);
            break;
        case sequence_based:
            f->pri = numeric_limits<int>::max() - _packet_seq_no[source];
            assert(f->pri >= 0);
            break;
        default:
            f->pri = 0;
        }
        if ( i == ( size - 1 ) ) { // Tail flit
            f->tail = true;
        } else {
            f->tail = false;
        }

        f->vc  = -1;

        if ( f->watch ) {
            *gWatchOut << GetSimTime() << " | "
                       << "node" << source << " | "
                       << "Enqueuing flit " << f->id
                       << " dest" <<f->dest
                       << " (packet " << f->pid
                       << ") at time " << time
                       << "." << endl;
        }

        _partial_packets[source][cl].push_back( f );


    }
}

void TrafficManager::_Inject(){



    for ( int input = 0; input < _nodes; ++input ) {
        for ( int c = 0; c < _classes; ++c ) {
            // Potentially generate packets for any (input,class)
            // that is currently empty

                bool generated = false;
                while( !generated && ( _qtime[input][c] <= _time ) ) {
                    int stype = _IssuePacket( input, c);



                    if ( stype != 0 ) { //generate a packet
                        _GeneratePacket( input, stype, c,
                                         _include_queuing==1 ?
                                         _qtime[input][c] : _time );
                                         packet_num++;
                                         //cout<<"gen"<<endl;
                        generated = true;
                    }

                    // stype=(RandomInt(10)<0&&GetSimTime()<15000);
                        // if(stype != 0){
                        //     _GeneratePacket( input, 10, c,
                        //                  _include_queuing==1 ?
                        //                  _qtime[input][c] : _time );
                        //                  packet_num++;
                        //                  //cout<<"gen"<<endl;
                        //     generated = true;
                        // }
                    // else{
                        // stype=(RandomInt(100)<10);
                        // if(stype != 0){
                        //     _GeneratePacket( input, 10, c,
                        //                  _include_queuing==1 ?
                        //                  _qtime[input][c] : _time );
                        //                  packet_num++;
                        //                  //cout<<"gen"<<endl;
                        //     generated = true;
                        // }
                    //     }
                    // }//testbigsize

                    // only advance time if this is not a reply packet
                    if(!_use_read_write[c] || (stype >= 0)){
                        ++_qtime[input][c];
                    }
                }

                if ( ( _sim_state == draining ) &&
                     ( _qtime[input][c] > _drain_time ) ) {
                    _qdrained[input][c] = true;
                }

        }
    }
}
vector<vector<int>> TrafficManager::findSwapsToTarget(vector<int>& nums) {
    vector<int> copy(nums.size(),0);
    for(int i=0;i<nums.size();i++){
        cout<<nums[i]<<" ";
        copy[i]=nums[i];
    }
    cout<<endl;
    std::vector<std::vector<int>> swaps;
        int n = copy.size();

        for (int i = 0; i < n; ++i) {
            while (copy[i] != i + 1) {
                int j = copy[i] - 1;
                std::swap(copy[i], copy[j]);
                swaps.push_back({i + 1, j + 1});
            }
        }

        return swaps;
}

void TrafficManager::_Step( )
{
    bool sample = true;



    for(int i=0;i<_node_num;i++){
        // if(i==0)cout<<phase_all_packet_num[group_phases[findgroup(i)]][i]<<endl;
        if(group_phases[findgroup(i)]>-1)
{
    // cout<<_time<<" id"<<i<<" "<<group_phases[findgroup(i)]<<endl;
        if(phase_nodes_a[group_phases[findgroup(i)]][i]>0)
        {
            phase_nodes_a[group_phases[findgroup(i)]][i]--;
        }
        if(phase_nodes_s1[group_phases[findgroup(i)]][i]>0)
        {
            phase_nodes_s1[group_phases[findgroup(i)]][i]--;
        }

   if(phase_nodes_s1[group_phases[findgroup(i)]][i]==0){
    //    cout<<group_phases[findgroup(i)]<<" "<<phase_all_packet_num[group_phases[findgroup(i)]][i]<<endl;
   // if(group_phases[findgroup(i)]==1&&findgroup(i)==2){cout<<"node "<<i<<" "<<phase_all_packet_num[group_phases[findgroup(i)]][i]<<endl;}

        // cout<<i<<" "<<phase_all_packet_num[group_phases[findgroup(i)]][i]<<endl;
        if(phase_all_packet_num[group_phases[findgroup(i)]][i]==0)
        {


        if(compute_mode==0){
            if(phase_nodes_a[group_phases[findgroup(i)]][i]==0){
                if(phase_nodes_s2[group_phases[findgroup(i)]][i]>0)
            {phase_nodes_s2[group_phases[findgroup(i)]][i]--;}
            }
            if(phase_nodes_s2[group_phases[findgroup(i)]][i]==0)
            {phase_nodes_s2[group_phases[findgroup(i)]][i]=-1;
            phase_nodes_s1[group_phases[findgroup(i)]][i]=-1;
            phase_nodes_a[group_phases[findgroup(i)]][i]=-1;
            phase_end_nodes[group_phases[findgroup(i)]][findgroup(i)]++;
            }
        }
        else{

                if(phase_nodes_s2[group_phases[findgroup(i)]][i]>0)
            {phase_nodes_s2[group_phases[findgroup(i)]][i]--;}

            if(phase_nodes_s2[group_phases[findgroup(i)]][i]==0)
            {
                if(phase_nodes_a[group_phases[findgroup(i)]][i]==0){
                phase_nodes_s2[group_phases[findgroup(i)]][i]=-1;
            phase_nodes_s1[group_phases[findgroup(i)]][i]=-1;
            phase_nodes_a[group_phases[findgroup(i)]][i]=-1;
            phase_end_nodes[group_phases[findgroup(i)]][findgroup(i)]++;
            // cout<<i<<" "<<findgroup(i)<<"---"<<phase_end_nodes[group_phases[findgroup(i)]][findgroup(i)]<<endl;

            }

            }

        }

            // if(phase_nodes_compute[group_phases[findgroup(i)]*2+1][i]>0)
            // {phase_nodes_compute[group_phases[findgroup(i)]*2+1][i]--;}
            // if(phase_nodes_compute[group_phases[findgroup(i)]*2+1][i]==0){
            // phase_nodes_compute[group_phases[findgroup(i)]*2][i]=-1;
            // phase_nodes_compute[group_phases[findgroup(i)]*2+1][i]=-1;
            // phase_end_nodes[group_phases[findgroup(i)]][findgroup(i)]++;
            // // if(findgroup(i)==21)
            // // {cout<<group_phases[findgroup(i)]<<" "<<phase_end_nodes[group_phases[findgroup(i)]][findgroup(i)]<<endl;}
            // cout<<phase_end_nodes[group_phases[findgroup(i)]][findgroup(i)]<<" "<<group_size[group_phases[findgroup(i)]][findgroup(i)]<<endl;
            // cout<<phase_end_nodes[group_phases[findgroup(i)]][findgroup(i)]<<' '<<group_size[group_phases[findgroup(i)]][findgroup(i)]<<endl;
            if(phase_end_nodes[group_phases[findgroup(i)]][findgroup(i)]==group_size[group_phases[findgroup(i)]][findgroup(i)])
            {

                if(group_phases[findgroup(i)]<_total_phase-1)
                {
                    group_phases[findgroup(i)]++;
                    cout<<GetSimTime()<<endl;

                    if(true){
                        int random_start ,random_end;

                        if(group_phases[findgroup(i)]>1)
                        {
                            familytime[(group_phases[findgroup(i)]-2)%4]=GetSimTime()-phase_time;
                            if((group_phases[findgroup(i)]-2)%4<2)
                            cout<<"父类"<<(group_phases[findgroup(i)]-2)%4+1<<"仿真时间为"<<GetSimTime()-phase_time<<endl;
                            else
                            cout<<"子类"<<(group_phases[findgroup(i)]-2)%4-1<<"仿真时间为"<<GetSimTime()-phase_time<<endl;

                        }

                        phase_time=GetSimTime();
                        if(group_phases[findgroup(i)]%4==1){
                            if(group_phases[findgroup(i)]>1){
                                auto min1 = std::min_element(familytime.begin(), familytime.end());
                                // 将最小元素的索引计算出来
                                int index1 = std::distance(familytime.begin(), min1);
                                for(int i=0;i<NODE_NUM;i++){
                                    nodeorder[0][i]=nodeorder[index1][i];
                                }
                                min_phase_num = familytime[index1];
                                cout<<"最小仿真周期为"<< min_phase_num<<endl;
                                if(index1<2)
                                cout<<"父类"<<index1+1<<"作为新的父类1"<<endl;
                                else cout<<"子类"<<index1-1<<"作为新的父类1"<<endl;
                                familytime[index1]=INT_MAX;
                                min1 = std::min_element(familytime.begin(), familytime.end());
                                // 将最小元素的索引计算出来
                                index1 = std::distance(familytime.begin(), min1);
                                for(int i=0;i<NODE_NUM;i++){
                                    nodeorder[1][i]=nodeorder[index1][i];
                                }
                                if(index1<2)
                                cout<<"父类"<<index1+1<<"作为新的父类2"<<endl;
                                else cout<<"子类"<<index1-1<<"作为新的父类2"<<endl;
                                if(nodeorder[1]==nodeorder[0]){
                                    cout<<"父类相同，将父类2随机打乱"<<endl;
                                }
                                auto currentTime = std::chrono::system_clock::now();
                                auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count();
                                srand(static_cast<unsigned>(4*timestamp));
                                random_start = std::rand() % NODE_NUM;
                                random_end = std::rand() % NODE_NUM;
                                while (random_start == random_end) {
                                    random_end = std::rand() % NODE_NUM;
                                }
                                if (random_start > random_end) {
                                    std::swap(random_start, random_end);
                                }
                                cout<<"父类2随机区间为"<<random_start<<" "<<random_end<<endl;
                                std::random_shuffle(nodeorder[1].begin() + random_start, nodeorder[1].begin() + random_end);


                            }
                            nodeorder[0].resize(NODE_NUM);
                            nodeorder[1].resize(NODE_NUM);
                            nodeorder[2].resize(NODE_NUM,0);
                            nodeorder[3].resize(NODE_NUM,0);
                            for(int i=0;i<NODE_NUM;i++){
                                nodeorder[2][i]=0;
                                nodeorder[3][i]=0;
                            }

                            auto currentTime = std::chrono::system_clock::now();
                            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count();
                            srand(static_cast<unsigned>(timestamp));
                            random_start = std::rand() % NODE_NUM;
                            random_end = std::rand() % NODE_NUM;
                            // 确保random_start和random_end不相等
                            while (random_start == random_end) {
                                random_end = std::rand() % NODE_NUM;
                            }
                            if (random_start > random_end) {
                                std::swap(random_start, random_end);
                            }
                            if(min_phase_num==0){
                                cout<<"初始化父类1和父类2"<<endl;
                                min_phase_num=1;
                                for (int i = 0; i < NODE_NUM; ++i) {
                                    nodeorder[0][i] = i+1; // 将数组的每个元素设置为其索引值
                                    nodeorder[1][i] = i+1;
                                }

                                std::random_device rd;
                                std::mt19937 rng(rd());
                                random_start = 0;
                                random_end = NODE_NUM-1;
                                // cout<<"父类1随机区间为"<<random_start<<" "<<random_end<<endl;
                                std::random_shuffle(nodeorder[0].begin() + random_start, nodeorder[0].begin() + random_end);

                                srand(static_cast<unsigned>(4*timestamp));
                                // random_start = std::rand() % NODE_NUM;
                                // random_end = std::rand() % NODE_NUM;
                                // while (random_start == random_end) {
                                //     random_end = std::rand() % NODE_NUM;
                                // }
                                // if (random_start > random_end) {
                                //     std::swap(random_start, random_end);
                                // }
                                // cout<<"父类2随机区间为"<<random_start<<" "<<random_end<<endl;
                                std::random_shuffle(nodeorder[1].begin() + random_start, nodeorder[1].begin() + random_end);
                            }
                            else{
                                familytime={0,0,0,0};
                            }

                            srand(static_cast<unsigned>(2*timestamp));
                            random_start = std::rand() % NODE_NUM;
                            random_end = std::rand() % NODE_NUM;
                            // 确保random_start和random_end不相等
                            while (random_start == random_end) {
                                random_end = std::rand() % NODE_NUM;
                            }
                            if (random_start > random_end) {
                                std::swap(random_start, random_end);
                            }
                            // cout<<"子类1的随机数为"<<random_start<<"和"<<random_end<<endl;
                            for (int i = random_start; i < random_end; ++i) {
                                nodeorder[2][i]=nodeorder[0][i];
                            }
                            cout<<"子类1继承父类1的区间为"<<random_start<<" "<<random_end<<endl;
                            int insert=0;
                            while(insert>= random_start&&insert< random_end){
                                insert++;
                            }

                            // 构建剩余部分，按照nodeorder[1]的顺序
                            for (int i = 0; i < NODE_NUM; ++i) {
                                if (insert<NODE_NUM&&std::find(nodeorder[2].begin(), nodeorder[2].end(), nodeorder[1][i]) == nodeorder[2].end()) {
                                    nodeorder[2][insert]=nodeorder[1][i];
                                    // cout<<insert<<" "<<nodeorder[2][insert]<<endl;

                                    insert++;
                                    while(insert>= random_start&&insert< random_end){
                                        insert++;
                                    }
                                }
                            }
                            srand(static_cast<unsigned>(3*timestamp));
                            random_start = std::rand() % NODE_NUM;
                            random_end = std::rand() % NODE_NUM;
                            // 确保random_start和random_end不相等
                            while (random_start == random_end) {
                                random_end = std::rand() % NODE_NUM;
                            }
                            if (random_start > random_end) {
                                std::swap(random_start, random_end);
                            }
                            // cout<<"子类2的随机数为"<<random_start<<"和"<<random_end<<endl;
                                for (int i = random_start; i < random_end; ++i) {
                                    nodeorder[3][i]=nodeorder[1][i];
                                }
                                cout<<"子类2继承父类2的区间为"<<random_start<<" "<<random_end<<endl;
                                insert=0;
                                while(insert>= random_start&&insert< random_end){
                                    insert++;
                                }
                                // cout<<"子类2的排列为："<<endl;
                                // for (int i = 0; i < NODE_NUM; ++i) {
                                //     std::cout << nodeorder[3][i] << " ";
                                // }

                                // 构建剩余部分，按照nodeorder[1]的顺序
                                for (int i = 0; i < NODE_NUM; ++i) {
                                    // cout<<nodeorder[0][i]<<endl;
                                    if (insert<NODE_NUM&&std::find(nodeorder[3].begin(), nodeorder[3].end(), nodeorder[0][i]) == nodeorder[3].end()) {
                                        nodeorder[3][insert]=nodeorder[0][i];
                                        // cout<<insert<<" "<<nodeorder[3][insert]<<endl;

                                        insert++;
                                        while(insert>= random_start&&insert< random_end){
                                            insert++;
                                        }
                                    }
                                }

                                // cout<<"子类1的排列为："<<endl;
                                // for (int i = 0; i < NODE_NUM; ++i) {
                                //     std::cout << nodeorder[2][i] << " ";
                                // }
                                // std::cout << std::endl;
                                // cout<<"子类2的排列为："<<endl;
                                // for (int i = 0; i < NODE_NUM; ++i) {
                                //     std::cout << nodeorder[3][i] << " ";
                                // }
                                // std::cout << std::endl;



                            // cout<<"父类1的排列为："<<endl;
                            //  for (int i = 0; i < NODE_NUM; ++i) {
                            //         std::cout << nodeorder[0][i] << " ";
                            //     }
                            //     std::cout << std::endl;
                            //     cout<<"父类2的排列为："<<endl;
                            //     for (int i = 0; i < NODE_NUM; ++i) {
                            //         std::cout << nodeorder[1][i] << " ";
                            //     }

                        }
                        swapnodes=findSwapsToTarget(nodeorder[(group_phases[findgroup(i)]-1)%4]);
                        // cout << "家庭成员"<<(group_phases[findgroup(i)]-1)%4<<"当前所有交换节点为";
                        // cout << "[";
                        // for (int i = 0; i < swapnodes.size(); ++i) {
                        //     cout << "[" << swapnodes[i][0] << "," << swapnodes[i][1] << "]";
                        //     if (i < swapnodes.size() - 1) {
                        //         cout << ",";
                        //     }
                        // }
                        // cout << "]" << endl;
                        for (vector<int>& row : nodedes[group_phases[findgroup(i)]]) {

                            for (int& dest : row) {
                                for(vector<int>& nodes :swapnodes){
                                if (dest == nodes[0]) {
                                    dest = nodes[1];
                                } else if (dest == nodes[1]) {
                                    dest = nodes[0];
                                }
                                }
                            }
                        }
                        for(vector<int>& nodes :swapnodes){
                            vector<int> temp = nodedes[group_phases[findgroup(i)]][nodes[0]];
                            nodedes[group_phases[findgroup(i)]][nodes[0]] = nodedes[group_phases[findgroup(i)]][nodes[1]];
                            nodedes[group_phases[findgroup(i)]][nodes[1]] = temp;
                            vector<int> temp1 = nodenum[group_phases[findgroup(i)]][nodes[0]];
                            nodenum[group_phases[findgroup(i)]][nodes[0]] = nodenum[group_phases[findgroup(i)]][nodes[1]];
                            nodenum[group_phases[findgroup(i)]][nodes[1]] = temp1;
                            int tempnum=phase_all_packet_num[group_phases[findgroup(i)]][nodes[0]];
                            phase_all_packet_num[group_phases[findgroup(i)]][nodes[0]]=phase_all_packet_num[group_phases[findgroup(i)]][nodes[1]];
                            phase_all_packet_num[group_phases[findgroup(i)]][nodes[1]]=tempnum;
                        }


                    }

                    // if(false){
                    // if(min_phase_num==0){min_phase_num=GetSimTime()-phase_time;}
                    // else{
                    //     auto currentTime = std::chrono::system_clock::now();
                    //     auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count();
                    //     srand(static_cast<unsigned>(timestamp));
                    //     int dt=GetSimTime()-phase_time-min_phase_num;


                    //     // 计算 P


                    //     if(dt<=0){
                    //         cout<<"接收交换"<<endl;
                    //         min_phase_num=GetSimTime()-phase_time;
                    //     }
                    //     else{
                    //         double P; // 概率
                    //         P = exp(- static_cast<double>(dt) / min_phase_num);
                    //         cout<<rand()<<endl;
                    //         double random_double = static_cast<double>(std::rand()) / RAND_MAX;
                    //         std::cout << "随机小数: " << random_double << "退火概率: " << P << std::endl;
                    //         if(random_double<P){
                    //             cout<<"接收交换"<<endl;
                    //             min_phase_num=GetSimTime()-phase_time;
                    //             }
                    //         else
                    //         swapnodes.pop_back();
                    //     }
                    //     }

                    // }
        //             if(false){
        //                 cout<<"当前最小周期："<<min_phase_num<<endl;
        //                 phase_time=GetSimTime();
        //                 cout<<GetSimTime()<<" group "<<findgroup(i)<<" enters phase "<<group_phases[findgroup(i)]<<endl;
        //                 auto currentTime = std::chrono::system_clock::now();

        //                 // 获取时间戳，以毫秒为单位
        //                 auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count();

        //                 srand(static_cast<unsigned>(timestamp));

        // // 生成两个不相同的随机数字在0到7之间
        //                 int random1 = rand() % NODE_NUM; // 0到7之间的随机数
        //                 int random2;
        //                 do {
        //                     random2 = rand() % NODE_NUM; // 0到7之间的随机数
        //                 } while (random2 == random1); // 确保两个数字不相同
        //                 cout<<"交换节点："<<random1<<" "<<random2<<endl;
        //                 vector<int> newSwap = {random1, random2};
        //                 swapnodes.push_back(newSwap);
        //                 cout << "当前所有交换节点为";
        //                 cout << "[";
        //                 for (int i = 0; i < swapnodes.size(); ++i) {
        //                     cout << "[" << swapnodes[i][0] << "," << swapnodes[i][1] << "]";
        //                     if (i < swapnodes.size() - 1) {
        //                         cout << ",";
        //                     }
        //                 }
        //                 cout << "]" << endl;

        //                 for (vector<int>& row : nodedes[group_phases[findgroup(i)]]) {

        //                     for (int& dest : row) {
        //                         for(vector<int>& nodes :swapnodes){
        //                         if (dest == nodes[0]) {
        //                             dest = nodes[1];
        //                         } else if (dest == nodes[1]) {
        //                             dest = nodes[0];
        //                         }
        //                         }
        //                     }
        //                 }
        //                 for(vector<int>& nodes :swapnodes){
        //                     vector<int> temp = nodedes[group_phases[findgroup(i)]][nodes[0]];
        //                     nodedes[group_phases[findgroup(i)]][nodes[0]] = nodedes[group_phases[findgroup(i)]][nodes[1]];
        //                     nodedes[group_phases[findgroup(i)]][nodes[1]] = temp;
        //                     vector<int> temp1 = nodenum[group_phases[findgroup(i)]][nodes[0]];
        //                     nodenum[group_phases[findgroup(i)]][nodes[0]] = nodenum[group_phases[findgroup(i)]][nodes[1]];
        //                     nodenum[group_phases[findgroup(i)]][nodes[1]] = temp1;
        //                     int tempnum=phase_all_packet_num[group_phases[findgroup(i)]][nodes[0]];
        //                     phase_all_packet_num[group_phases[findgroup(i)]][nodes[0]]=phase_all_packet_num[group_phases[findgroup(i)]][nodes[1]];
        //                     phase_all_packet_num[group_phases[findgroup(i)]][nodes[1]]=tempnum;
        //                 }
        //             }
                }
                    else{
                        if(!sample){
                        group_phases[findgroup(i)]=0;
                        }
                        else{group_phases[findgroup(i)]=-1;
                         }
                       cout<<"TIME "<<GetSimTime()<<" group "<<findgroup(i)<<" finishes all phases."<<endl;
                       group_time[findgroup(i)]=GetSimTime();
                    }
                }

        }
        }

}
      }
      bool tempflag=false;
      for(int i=0;i<group_num;i++){
          if(group_phases[i]>-1)tempflag=true;
      }
     if(!sample||tempflag)
    {


    // for(int i=0;i<int(group_phases.size());i++){
    //     while(group_phases[i]<_total_phase-1&&group_size[group_phases[i]][i]==0)
    //     {
    //         cout<<"group "<<i<<" enters phase "<<group_phases[i]<<endl;
    //         if(group_phases[i]<_total_phase-1){group_phases[i]++;}else break;
    //     }
    // }

    bool flits_in_flight = false;
    for(int c = 0; c < _classes; ++c) {
        flits_in_flight |= !_total_in_flight_flits[c].empty();
    }
    if(flits_in_flight && (_deadlock_timer++ >= _deadlock_warn_timeout)){
        _deadlock_timer = 0;
        cout << "WARNING: Possible network deadlock.\n";
                map<int, Flit *>::const_iterator iter;
            // for(iter = _total_in_flight_flits[0].begin();
            //     iter != _total_in_flight_flits[0].end();
            //     iter++) {
            //     cout<<iter->second->src<<" "<<iter->second->dest<<endl;
            // }
    }

    vector<map<int, Flit *> > flits(_subnets);


    for ( int subnet = 0; subnet < _subnets; ++subnet ) {

        for ( int n = 0; n < _nodes; ++n ) {
            Flit * const f = _net[subnet]->ReadFlit( n );

            if ( f ) {

                if(f->watch) {
                    *gWatchOut << GetSimTime() << " | "
                               << "node" << n << " | "
                               << "Ejecting flit " << f->id
                               << " src " << f->src << ")"
                               << " from VC " << f->vc
                               << "." << endl;
                 }
                flits[subnet].insert(make_pair(n, f));
                if((_sim_state == warming_up) || (_sim_state == running)) {
                    ++_accepted_flits[f->cl][n];
                    if(f->tail) {
                        ++_accepted_packets[f->cl][n];
                    }
                }
            }

            Credit * const c = _net[subnet]->ReadCredit( n );
            if ( c ) {
#ifdef TRACK_FLOWS
                for(set<int>::const_iterator iter = c->vc.begin(); iter != c->vc.end(); ++iter) {
                    int const vc = *iter;
                    assert(!_outstanding_classes[n][subnet][vc].empty());
                    int cl = _outstanding_classes[n][subnet][vc].front();
                    _outstanding_classes[n][subnet][vc].pop();
                    assert(_outstanding_credits[cl][subnet][n] > 0);
                    --_outstanding_credits[cl][subnet][n];
                }
#endif
                _buf_states[n][subnet]->ProcessCredit(c);
                c->Free();
            }

        }
        _net[subnet]->ReadInputs( );

    }



    if ( !_empty_network ) {
        _Inject();
    }




    for(int subnet = 0; subnet < _subnets; ++subnet) {

        for(int n = 0; n < _nodes; ++n) {


            Flit * f = NULL;

            BufferState * const dest_buf = _buf_states[n][subnet];

            int const last_class = _last_class[n][subnet];

            int class_limit = _classes;

            if(_hold_switch_for_packet) {
                list<Flit *> const & pp = _partial_packets[n][last_class];
                if(!pp.empty() && !pp.front()->head &&
                   !dest_buf->IsFullFor(pp.front()->vc)) {
                    f = pp.front();
                    assert(f->vc == _last_vc[n][subnet][last_class]);

                    // if we're holding the connection, we don't need to check that class
                    // again in the for loop
                    --class_limit;
                }
            }


            for(int i = 1; i <= class_limit; ++i) {

                int const c = (last_class + i) % _classes;

                list<Flit *> const & pp = _partial_packets[n][c];

                if(pp.empty()) {
                    continue;
                }

                Flit * const cf = pp.front();
                assert(cf);
                assert(cf->cl == c);

                if(cf->subnetwork != subnet) {
                    continue;
                }

                if(f && (f->pri >= cf->pri)) {
                    continue;
                }

                if(cf->head && cf->vc == -1) { // Find first available VC

                    OutputSet route_set;
                    _rf(NULL, cf, -1, &route_set, true);
                    set<OutputSet::sSetElement> const & os = route_set.GetSet();
                    assert(os.size() == 1);
                    OutputSet::sSetElement const & se = *os.begin();
                    assert(se.output_port == -1);
                    int vc_start = se.vc_start;
                    int vc_end = se.vc_end;
                    int vc_count = vc_end - vc_start + 1;
                    if(_noq) {
                        assert(_lookahead_routing);
                        const FlitChannel * inject = _net[subnet]->GetInject(n);
                        const Router * router = inject->GetSink();
                        assert(router);
                        int in_channel = inject->GetSinkPort();

                        // NOTE: Because the lookahead is not for injection, but for the
                        // first hop, we have to temporarily set cf's VC to be non-negative
                        // in order to avoid seting of an assertion in the routing function.
                        cf->vc = vc_start;
                        _rf(router, cf, in_channel, &cf->la_route_set, false);
                        cf->vc = -1;

                        if(cf->watch) {
                            *gWatchOut << GetSimTime() << " | "
                                       << "node" << n << " | "
                                       << "Generating lookahead routing info for flit " << cf->id
                                       << " (NOQ)." << endl;
                        }
                        set<OutputSet::sSetElement> const sl = cf->la_route_set.GetSet();
                        assert(sl.size() == 1);
                        int next_output = sl.begin()->output_port;
                        vc_count /= router->NumOutputs();
                        vc_start += next_output * vc_count;
                        vc_end = vc_start + vc_count - 1;
                        assert(vc_start >= se.vc_start && vc_start <= se.vc_end);
                        assert(vc_end >= se.vc_start && vc_end <= se.vc_end);
                        assert(vc_start <= vc_end);
                    }
                    if(cf->watch) {
                        *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                   << "Finding output VC for flit " << cf->id
                                   << ":" << endl;
                    }
                    for(int i = 1; i <= vc_count; ++i) {
                        int const lvc = _last_vc[n][subnet][c];
                        int const vc =
                            (lvc < vc_start || lvc > vc_end) ?
                            vc_start :
                            (vc_start + (lvc - vc_start + i) % vc_count);
                        assert((vc >= vc_start) && (vc <= vc_end));
                        if(!dest_buf->IsAvailableFor(vc)) {
                            if(cf->watch) {
                                *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                           << "  Output VC " << vc << " is busy." << endl;
                            }
                        } else {
                            if(dest_buf->IsFullFor(vc)) {
                                if(cf->watch) {
                                    *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                               << "  Output VC " << vc << " is full." << endl;
                                }
                            } else {
                                if(cf->watch) {
                                    *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                               << "  Selected output VC " << vc << "." << endl;
                                }
                                cf->vc = vc;
                                break;
                            }
                        }
                    }
                }

                if(cf->vc == -1) {
                    if(cf->watch) {
                        *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                   << "No output VC found for flit " << cf->id
                                   << "." << endl;
                    }
                } else {
                    if(dest_buf->IsFullFor(cf->vc)) {
                        if(cf->watch) {
                            *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                       << "Selected output VC " << cf->vc
                                       << " is full for flit " << cf->id
                                       << "." << endl;
                        }
                    } else {
                        f = cf;
                    }
                }
            }

            if(f) {

                assert(f->subnetwork == subnet);

                int const c = f->cl;

                if(f->head) {

                    if (_lookahead_routing) {
                        if(!_noq) {
                            const FlitChannel * inject = _net[subnet]->GetInject(n);
                            const Router * router = inject->GetSink();
                            assert(router);
                            int in_channel = inject->GetSinkPort();
                            _rf(router, f, in_channel, &f->la_route_set, false);
                            if(f->watch) {
                                *gWatchOut << GetSimTime() << " | "
                                           << "node" << n << " | "
                                           << "Generating lookahead routing info for flit " << f->id
                                           << "." << endl;
                            }
                        } else if(f->watch) {
                            *gWatchOut << GetSimTime() << " | "
                                       << "node" << n << " | "
                                       << "Already generated lookahead routing info for flit " << f->id
                                       << " (NOQ)." << endl;
                        }
                    } else {
                        f->la_route_set.Clear();
                    }

                    dest_buf->TakeBuffer(f->vc);
                    _last_vc[n][subnet][c] = f->vc;
                }

                _last_class[n][subnet] = c;

                _partial_packets[n][c].pop_front();




#ifdef TRACK_FLOWS
                ++_outstanding_credits[c][subnet][n];
                _outstanding_classes[n][subnet][f->vc].push(c);
#endif

                dest_buf->SendingFlit(f);

                if(_pri_type == network_age_based) {
                    f->pri = numeric_limits<int>::max() - _time;
                    assert(f->pri >= 0);
                }

                if(f->watch) {
                    *gWatchOut << GetSimTime() << " | "
                               << "node" << n << " | "
                               << "Injecting flit " << f->id
                               << " into subnet " << subnet
                               << " at time " << _time
                               << " with priority " << f->pri
                               << "." << endl;
                }
                f->itime = _time;

                // Pass VC "back"
                if(!_partial_packets[n][c].empty() && !f->tail) {
                    Flit * const nf = _partial_packets[n][c].front();
                    nf->vc = f->vc;
                }

                if((_sim_state == warming_up) || (_sim_state == running)) {
                    ++_sent_flits[c][n];
                    if(f->head) {
                        ++_sent_packets[c][n];
                    }
                }

#ifdef TRACK_FLOWS
                ++_injected_flits[c][n];
#endif

                _net[subnet]->WriteFlit(f, n);

            }
        }
    }

    for(int subnet = 0; subnet < _subnets; ++subnet) {
        for(int n = 0; n < _nodes; ++n) {

            map<int, Flit *>::const_iterator iter = flits[subnet].find(n);
            if(iter != flits[subnet].end()) {
                Flit * const f = iter->second;
              //  cout<<f->id<<endl;

                f->atime = _time;
                if(f->watch) {
                    *gWatchOut << GetSimTime() << " | "
                               << "node" << n << " | "
                               << "Injecting credit for VC " << f->vc
                               << " into subnet " << subnet
                               << "." << endl;
                }
                Credit * const c = Credit::New();
                c->vc.insert(f->vc);
                _net[subnet]->WriteCredit(c, n);

#ifdef TRACK_FLOWS
                ++_ejected_flits[f->cl][n];
#endif

                _RetireFlit(f, n);
            }
        }
        flits[subnet].clear();
        _net[subnet]->Evaluate( );
        _net[subnet]->WriteOutputs( );
    }
    run_time++;
    ++_time;
    assert(_time);
    if(gTrace){
        cout<<"TIME "<<_time<<endl;
    }
    if(_time%measure_step==measure_step-1&&(_time/measure_step<24))
    {
        export_data(_time);
        for(int j=0;j<group_phases.size();j++){
                ofs_phase<<"groupphase["<<_time/measure_step<<"]["<<j<<"]="<<group_phases[j]<<";"<<endl;
            }
            if(_time/measure_step==23)ofs_phase<<"export default groupphase;"<<endl;

    }
    }


}

bool TrafficManager::_PacketsOutstanding( ) const
{
    for ( int c = 0; c < _classes; ++c ) {
        if ( _measure_stats[c] ) {
            if ( _measured_in_flight_flits[c].empty() ) {

                for ( int s = 0; s < _nodes; ++s ) {
                    if ( !_qdrained[s][c] ) {
#ifdef DEBUG_DRAIN
                        cout << "waiting on queue " << s << " class " << c;
                        cout << ", time = " << _time << " qtime = " << _qtime[s][c] << endl;
#endif
                        return true;
                    }
                }
            } else {
#ifdef DEBUG_DRAIN
                cout << "in flight = " << _measured_in_flight_flits[c].size() << endl;
#endif
                return true;
            }
        }
    }
    return false;
}

void TrafficManager::_ClearStats( )
{
    _slowest_flit.assign(_classes, -1);
    _slowest_packet.assign(_classes, -1);

    for ( int c = 0; c < _classes; ++c ) {

        _plat_stats[c]->Clear( );
        _nlat_stats[c]->Clear( );
        _flat_stats[c]->Clear( );

        _frag_stats[c]->Clear( );

        _sent_packets[c].assign(_nodes, 0);
        _accepted_packets[c].assign(_nodes, 0);
        _sent_flits[c].assign(_nodes, 0);
        _accepted_flits[c].assign(_nodes, 0);

#ifdef TRACK_STALLS
        _buffer_busy_stalls[c].assign(_subnets*_routers, 0);
        _buffer_conflict_stalls[c].assign(_subnets*_routers, 0);
        _buffer_full_stalls[c].assign(_subnets*_routers, 0);
        _buffer_reserved_stalls[c].assign(_subnets*_routers, 0);
        _crossbar_conflict_stalls[c].assign(_subnets*_routers, 0);
#endif
        if(_pair_stats){
            for ( int i = 0; i < _nodes; ++i ) {
                for ( int j = 0; j < _nodes; ++j ) {
                    _pair_plat[c][i*_nodes+j]->Clear( );
                    _pair_nlat[c][i*_nodes+j]->Clear( );
                    _pair_flat[c][i*_nodes+j]->Clear( );
                }
            }
        }
        _hop_stats[c]->Clear();

    }

    _reset_time = _time;
}

void TrafficManager::_ComputeStats( const vector<int> & stats, int *sum, int *min, int *max, int *min_pos, int *max_pos ) const
{
    int const count = stats.size();
    assert(count > 0);

    if(min_pos) {
        *min_pos = 0;
    }
    if(max_pos) {
        *max_pos = 0;
    }

    if(min) {
        *min = stats[0];
    }
    if(max) {
        *max = stats[0];
    }

    *sum = stats[0];

    for ( int i = 1; i < count; ++i ) {
        int curr = stats[i];
        if ( min  && ( curr < *min ) ) {
            *min = curr;
            if ( min_pos ) {
                *min_pos = i;
            }
        }
        if ( max && ( curr > *max ) ) {
            *max = curr;
            if ( max_pos ) {
                *max_pos = i;
            }
        }
        *sum += curr;
    }
}

void TrafficManager::_DisplayRemaining( ostream & os ) const
{
    for(int c = 0; c < _classes; ++c) {

        map<int, Flit *>::const_iterator iter;
        int i;

        os << "Class " << c << ":" << endl;

        os << "Remaining flits: ";
        for ( iter = _total_in_flight_flits[c].begin( ), i = 0;
              ( iter != _total_in_flight_flits[c].end( ) ) && ( i < 10 );
              iter++, i++ ) {
            os << iter->first << " ";
        }
        if(_total_in_flight_flits[c].size() > 10)
            os << "[...] ";

        os << "(" << _total_in_flight_flits[c].size() << " flits)" << endl;

        os << "Measured flits: ";
        for ( iter = _measured_in_flight_flits[c].begin( ), i = 0;
              ( iter != _measured_in_flight_flits[c].end( ) ) && ( i < 10 );
              iter++, i++ ) {
            os << iter->first << " ";
        }
        if(_measured_in_flight_flits[c].size() > 10)
            os << "[...] ";

        os << "(" << _measured_in_flight_flits[c].size() << " flits)" << endl;

    }
}

bool TrafficManager::_SingleSim( )
{
    int converged = 0;

    //once warmed up, we require 3 converging runs to end the simulation
    vector<double> prev_latency(_classes, 0.0);
    vector<double> prev_accepted(_classes, 0.0);
    bool clear_last = false;
    for (int j=0;j<_total_phase;j++)
    {
        for(int i=0;i<_nodes;i++){
            //if(phase_all_packet_num[j][i]>0)
        group_size[j][findgroup(i)]++;
    }
    }

    total_phases = 0;
    while ( total_phases < _total_phase )
        {

        if ( clear_last  ) {
            clear_last = false;
            _ClearStats( );
        }

        while(total_phases < _total_phase)
        { _Step( );
        int test=0;
        for(int i=0;i<_nodes;i++){

           if(!sample){ if(LUT[total_phases][i].size()>0){test=1;}}
            else if(nodedes[total_phases][i].size()>0){
                test=1;break;}
            // cout<<phase_all_packet_num[5][i]<<" ";
        }
        // cout<<endl;
        if(test==0){total_phases++;
cout <<GetSimTime()<<" All packets in phase " <<total_phases-1<<" sent out"<< endl;}
        }


        cout <<GetSimTime()<<" All packets injected" << endl;
        cout<<group_phases.size()<<endl;
        bool sample = true;
if(!sample){
        for(int i=0;i<group_phases.size();i++)
        {

            while(group_phases[i]!=0)
            {
                _Step( );
                }

        }
}


        UpdateStats();
        DisplayStats();

        int lat_exc_class = -1;
        int lat_chg_exc_class = -1;
        int acc_chg_exc_class = -1;

        for(int c = 0; c < _classes; ++c) {

            if(_measure_stats[c] == 0) {
                continue;
            }

            double cur_latency = _plat_stats[c]->Average( );

            int total_accepted_count;
            _ComputeStats( _accepted_flits[c], &total_accepted_count );
            double total_accepted_rate = (double)total_accepted_count / (double)(_time - _reset_time);
            double cur_accepted = total_accepted_rate / (double)_nodes;

            double latency_change = fabs((cur_latency - prev_latency[c]) / cur_latency);
            prev_latency[c] = cur_latency;

            double accepted_change = fabs((cur_accepted - prev_accepted[c]) / cur_accepted);
            prev_accepted[c] = cur_accepted;

            double latency = (double)_plat_stats[c]->Sum();
            double count = (double)_plat_stats[c]->NumSamples();

              map<int, Flit *>::const_iterator iter;
            for(iter = _total_in_flight_flits[c].begin();
                iter != _total_in_flight_flits[c].end();
                iter++) {
                latency += (double)(_time - iter->second->ctime);
                count++;
            }

            if((lat_exc_class < 0) &&
               (_latency_thres[c] >= 0.0) &&
               ((latency / count) > _latency_thres[c])) {
                lat_exc_class = c;
            }

            cout << "latency change    = " << latency_change << endl;
            if(lat_chg_exc_class < 0) {
                if((_sim_state == running) &&
                          (_stopping_threshold[c] >= 0.0) &&
                          (latency_change > _stopping_threshold[c])) {
                    lat_chg_exc_class = c;
                }
            }

            cout << "throughput change = " << accepted_change << endl;
            if(acc_chg_exc_class < 0) {
                if((_sim_state == running) &&
                          (_acc_stopping_threshold[c] >= 0.0) &&
                          (accepted_change > _acc_stopping_threshold[c])) {
                    acc_chg_exc_class = c;
                }
            }

        }

        // Fail safe for latency mode, throughput will ust continue
        if ( _measure_latency && ( lat_exc_class >= 0 ) ) {

            cout << "Average latency for class " << lat_exc_class << " exceeded " << _latency_thres[lat_exc_class] << " cycles. Aborting simulation." << endl;
            converged = 0;
            _sim_state = draining;
            _drain_time = _time;
            if(_stats_out) {
                WriteStats(*_stats_out);
            }
            break;

        }
        cout << "Time taken is " << _time << " cycles" <<endl;
        cout<<"send packets "<<packet_num<<endl;

        if(_sim_state == running) {
            if ( ( !_measure_latency || ( lat_chg_exc_class < 0 ) ) &&
                 ( acc_chg_exc_class < 0 ) ) {
                ++converged;
            } else {
                converged = 0;
            }
        }


        //wait until all the credits are drained as well
        if(sample){
            int flag=0;
            while(flag!=group_num){
                flag=0;
                for(int i=0;i<group_num;i++)
                {if(group_phases[i]!=0)flag++;
                }
             _Step();

        }
        }
        if(!sample)
        {while(Credit::OutStanding()>0){
            _Step();
        }
        }
        _empty_network = false;

        //for the love of god don't ever say "Time taken" anywhere else
        //the power script depend on it


        if(_stats_out) {
            WriteStats(*_stats_out);
        }

    }

    if ( _sim_state == running ) {
        ++converged;

        _sim_state  = draining;
        _drain_time = _time;

        if ( _measure_latency ) {
            cout << "Draining all recorded packets ..." << endl;
            int empty_steps = 0;
            while( _PacketsOutstanding( ) ) {
                _Step( );

                ++empty_steps;

                if ( empty_steps % 1000 == 0 ) {

                    int lat_exc_class = -1;

                    for(int c = 0; c < _classes; c++) {

                        double threshold = _latency_thres[c];

                        if(threshold < 0.0) {
                            continue;
                        }

                        double acc_latency = _plat_stats[c]->Sum();
                        double acc_count = (double)_plat_stats[c]->NumSamples();

                        map<int, Flit *>::const_iterator iter;
                        for(iter = _total_in_flight_flits[c].begin();
                            iter != _total_in_flight_flits[c].end();
                            iter++) {
                            acc_latency += (double)(_time - iter->second->ctime);
                            acc_count++;
                        }

                        if((acc_latency / acc_count) > threshold) {
                            lat_exc_class = c;
                            break;
                        }
                    }

                    if(lat_exc_class >= 0) {
                        cout << "Average latency for class " << lat_exc_class << " exceeded " << _latency_thres[lat_exc_class] << " cycles. Aborting simulation." << endl;
                        converged = 0;
                        _sim_state = warming_up;
                        if(_stats_out) {
                            WriteStats(*_stats_out);
                        }
                        break;
                    }

                    _DisplayRemaining( );

                }
            }
        }
    } else {
        cout << "Too many sample periods needed to converge" << endl;
    }

    return ( converged > 0 );
}

bool TrafficManager::Run( )
{
    for ( int sim = 0; sim < _total_sims; ++sim ) {

        _time = 0;

        //remove any pending request from the previous simulations
        _requestsOutstanding.assign(_nodes, 0);
        for (int i=0;i<_nodes;i++) {
            while(!_repliesPending[i].empty()) {
                _repliesPending[i].front()->Free();
                _repliesPending[i].pop_front();
            }
        }

        //reset queuetime for all sources
        for ( int s = 0; s < _nodes; ++s ) {
            _qtime[s].assign(_classes, 0);
            _qdrained[s].assign(_classes, false);
        }

        // warm-up ...
        // reset stats, all packets after warmup_time marked
        // converge
        // draing, wait until all packets finish
        _sim_state    = running;

        _ClearStats( );

        for(int c = 0; c < _classes; ++c) {
            _traffic_pattern[c]->reset();
            _injection_process[c]->reset();
        }

        if ( !_SingleSim( ) ) {
            cout << "Simulation unstable, ending ..." << endl;
            return false;
        }
        // cout<<_time<<endl;

        // Empty any remaining packets

        _UpdateOverallStats();
    }

    DisplayOverallStats();
    if(_print_csv_results) {
        DisplayOverallStatsCSV();
    }

    return true;
}

void TrafficManager::_UpdateOverallStats() {
    for ( int c = 0; c < _classes; ++c ) {

        if(_measure_stats[c] == 0) {
            continue;
        }

        _overall_min_plat[c] += _plat_stats[c]->Min();
        _overall_avg_plat[c] += _plat_stats[c]->Average();
        _overall_max_plat[c] += _plat_stats[c]->Max();
        _overall_min_nlat[c] += _nlat_stats[c]->Min();
        _overall_avg_nlat[c] += _nlat_stats[c]->Average();
        _overall_max_nlat[c] += _nlat_stats[c]->Max();
        _overall_min_flat[c] += _flat_stats[c]->Min();
        _overall_avg_flat[c] += _flat_stats[c]->Average();
        _overall_max_flat[c] += _flat_stats[c]->Max();

        _overall_min_frag[c] += _frag_stats[c]->Min();
        _overall_avg_frag[c] += _frag_stats[c]->Average();
        _overall_max_frag[c] += _frag_stats[c]->Max();

        _overall_hop_stats[c] += _hop_stats[c]->Average();

        int count_min, count_sum, count_max;
        double rate_min, rate_sum, rate_max;
        double rate_avg;
        double time_delta = (double)(_drain_time - _reset_time);
        _ComputeStats( _sent_flits[c], &count_sum, &count_min, &count_max );
        rate_min = (double)count_min / time_delta;
        rate_sum = (double)count_sum / time_delta;
        rate_max = (double)count_max / time_delta;
        rate_avg = rate_sum / (double)_nodes;
        _overall_min_sent[c] += rate_min;
        _overall_avg_sent[c] += rate_avg;
        _overall_max_sent[c] += rate_max;
        _ComputeStats( _sent_packets[c], &count_sum, &count_min, &count_max );
        rate_min = (double)count_min / time_delta;
        rate_sum = (double)count_sum / time_delta;
        rate_max = (double)count_max / time_delta;
        rate_avg = rate_sum / (double)_nodes;
        _overall_min_sent_packets[c] += rate_min;
        _overall_avg_sent_packets[c] += rate_avg;
        _overall_max_sent_packets[c] += rate_max;
        _ComputeStats( _accepted_flits[c], &count_sum, &count_min, &count_max );
        rate_min = (double)count_min / time_delta;
        rate_sum = (double)count_sum / time_delta;
        rate_max = (double)count_max / time_delta;
        rate_avg = rate_sum / (double)_nodes;
        _overall_min_accepted[c] += rate_min;
        _overall_avg_accepted[c] += rate_avg;
        _overall_max_accepted[c] += rate_max;
        _ComputeStats( _accepted_packets[c], &count_sum, &count_min, &count_max );
        rate_min = (double)count_min / time_delta;
        rate_sum = (double)count_sum / time_delta;
        rate_max = (double)count_max / time_delta;
        rate_avg = rate_sum / (double)_nodes;
        _overall_min_accepted_packets[c] += rate_min;
        _overall_avg_accepted_packets[c] += rate_avg;
        _overall_max_accepted_packets[c] += rate_max;

#ifdef TRACK_STALLS
        _ComputeStats(_buffer_busy_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        _overall_buffer_busy_stalls[c] += rate_avg;
        _ComputeStats(_buffer_conflict_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        _overall_buffer_conflict_stalls[c] += rate_avg;
        _ComputeStats(_buffer_full_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        _overall_buffer_full_stalls[c] += rate_avg;
        _ComputeStats(_buffer_reserved_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        _overall_buffer_reserved_stalls[c] += rate_avg;
        _ComputeStats(_crossbar_conflict_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        _overall_crossbar_conflict_stalls[c] += rate_avg;
#endif

    }
}

void TrafficManager::WriteStats(ostream & os) const {

    os << "%=================================" << endl;

    for(int c = 0; c < _classes; ++c) {

        if(_measure_stats[c] == 0) {
            continue;
        }

        //c+1 due to matlab array starting at 1
        os << "plat(" << c+1 << ") = " << _plat_stats[c]->Average() << ";" << endl
           << "plat_hist(" << c+1 << ",:) = " << *_plat_stats[c] << ";" << endl
           << "nlat(" << c+1 << ") = " << _nlat_stats[c]->Average() << ";" << endl
           << "nlat_hist(" << c+1 << ",:) = " << *_nlat_stats[c] << ";" << endl
           << "flat(" << c+1 << ") = " << _flat_stats[c]->Average() << ";" << endl
           << "flat_hist(" << c+1 << ",:) = " << *_flat_stats[c] << ";" << endl
           << "frag_hist(" << c+1 << ",:) = " << *_frag_stats[c] << ";" << endl
           << "hops(" << c+1 << ",:) = " << *_hop_stats[c] << ";" << endl;
        if(_pair_stats){
            os<< "pair_sent(" << c+1 << ",:) = [ ";
            for(int i = 0; i < _nodes; ++i) {
                for(int j = 0; j < _nodes; ++j) {
                    os << _pair_plat[c][i*_nodes+j]->NumSamples() << " ";
                }
            }
            os << "];" << endl
               << "pair_plat(" << c+1 << ",:) = [ ";
            for(int i = 0; i < _nodes; ++i) {
                for(int j = 0; j < _nodes; ++j) {
                    os << _pair_plat[c][i*_nodes+j]->Average( ) << " ";
                }
            }
            os << "];" << endl
               << "pair_nlat(" << c+1 << ",:) = [ ";
            for(int i = 0; i < _nodes; ++i) {
                for(int j = 0; j < _nodes; ++j) {
                    os << _pair_nlat[c][i*_nodes+j]->Average( ) << " ";
                }
            }
            os << "];" << endl
               << "pair_flat(" << c+1 << ",:) = [ ";
            for(int i = 0; i < _nodes; ++i) {
                for(int j = 0; j < _nodes; ++j) {
                    os << _pair_flat[c][i*_nodes+j]->Average( ) << " ";
                }
            }
        }

        double time_delta = (double)(_drain_time - _reset_time);

        os << "];" << endl
           << "sent_packets(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _nodes; ++d ) {
            os << (double)_sent_packets[c][d] / time_delta << " ";
        }
        os << "];" << endl
           << "accepted_packets(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _nodes; ++d ) {
            os << (double)_accepted_packets[c][d] / time_delta << " ";
        }
        os << "];" << endl
           << "sent_flits(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _nodes; ++d ) {
            os << (double)_sent_flits[c][d] / time_delta << " ";
        }
        os << "];" << endl
           << "accepted_flits(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _nodes; ++d ) {
            os << (double)_accepted_flits[c][d] / time_delta << " ";
        }
        os << "];" << endl
           << "sent_packet_size(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _nodes; ++d ) {
            os << (double)_sent_flits[c][d] / (double)_sent_packets[c][d] << " ";
        }
        os << "];" << endl
           << "accepted_packet_size(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _nodes; ++d ) {
            os << (double)_accepted_flits[c][d] / (double)_accepted_packets[c][d] << " ";
        }
        os << "];" << endl;
#ifdef TRACK_STALLS
        os << "buffer_busy_stalls(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _subnets*_routers; ++d ) {
            os << (double)_buffer_busy_stalls[c][d] / time_delta << " ";
        }
        os << "];" << endl
           << "buffer_conflict_stalls(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _subnets*_routers; ++d ) {
            os << (double)_buffer_conflict_stalls[c][d] / time_delta << " ";
        }
        os << "];" << endl
           << "buffer_full_stalls(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _subnets*_routers; ++d ) {
            os << (double)_buffer_full_stalls[c][d] / time_delta << " ";
        }
        os << "];" << endl
           << "buffer_reserved_stalls(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _subnets*_routers; ++d ) {
            os << (double)_buffer_reserved_stalls[c][d] / time_delta << " ";
        }
        os << "];" << endl
           << "crossbar_conflict_stalls(" << c+1 << ",:) = [ ";
        for ( int d = 0; d < _subnets*_routers; ++d ) {
            os << (double)_crossbar_conflict_stalls[c][d] / time_delta << " ";
        }
        os << "];" << endl;
#endif
    }
}

void TrafficManager::UpdateStats() {
#if defined(TRACK_FLOWS) || defined(TRACK_STALLS)
    for(int c = 0; c < _classes; ++c) {
#ifdef TRACK_FLOWS
        {
            char trail_char = (c == _classes - 1) ? '\n' : ',';
            if(_injected_flits_out) *_injected_flits_out << _injected_flits[c] << trail_char;
            _injected_flits[c].assign(_nodes, 0);
            if(_ejected_flits_out) *_ejected_flits_out << _ejected_flits[c] << trail_char;
            _ejected_flits[c].assign(_nodes, 0);
        }
#endif
        for(int subnet = 0; subnet < _subnets; ++subnet) {
#ifdef TRACK_FLOWS
            if(_outstanding_credits_out) *_outstanding_credits_out << _outstanding_credits[c][subnet] << ',';
            if(_stored_flits_out) *_stored_flits_out << vector<int>(_nodes, 0) << ',';
#endif
            for(int router = 0; router < _routers; ++router) {
                Router * const r = _router[subnet][router];
#ifdef TRACK_FLOWS
                char trail_char =
                    ((router == _routers - 1) && (subnet == _subnets - 1) && (c == _classes - 1)) ? '\n' : ',';
                if(_received_flits_out) *_received_flits_out << r->GetReceivedFlits(c) << trail_char;
                if(_stored_flits_out) *_stored_flits_out << r->GetStoredFlits(c) << trail_char;
                if(_sent_flits_out) *_sent_flits_out << r->GetSentFlits(c) << trail_char;
                if(_outstanding_credits_out) *_outstanding_credits_out << r->GetOutstandingCredits(c) << trail_char;
                if(_active_packets_out) *_active_packets_out << r->GetActivePackets(c) << trail_char;
                r->ResetFlowStats(c);
#endif
#ifdef TRACK_STALLS
                _buffer_busy_stalls[c][subnet*_routers+router] += r->GetBufferBusyStalls(c);
                _buffer_conflict_stalls[c][subnet*_routers+router] += r->GetBufferConflictStalls(c);
                _buffer_full_stalls[c][subnet*_routers+router] += r->GetBufferFullStalls(c);
                _buffer_reserved_stalls[c][subnet*_routers+router] += r->GetBufferReservedStalls(c);
                _crossbar_conflict_stalls[c][subnet*_routers+router] += r->GetCrossbarConflictStalls(c);
                r->ResetStallStats(c);
#endif
            }
        }
    }
#ifdef TRACK_FLOWS
    if(_injected_flits_out) *_injected_flits_out << flush;
    if(_received_flits_out) *_received_flits_out << flush;
    if(_stored_flits_out) *_stored_flits_out << flush;
    if(_sent_flits_out) *_sent_flits_out << flush;
    if(_outstanding_credits_out) *_outstanding_credits_out << flush;
    if(_ejected_flits_out) *_ejected_flits_out << flush;
    if(_active_packets_out) *_active_packets_out << flush;
#endif
#endif

#ifdef TRACK_CREDITS
    for(int s = 0; s < _subnets; ++s) {
        for(int n = 0; n < _nodes; ++n) {
            BufferState const * const bs = _buf_states[n][s];
            for(int v = 0; v < _vcs; ++v) {
                if(_used_credits_out) *_used_credits_out << bs->OccupancyFor(v) << ',';
                if(_free_credits_out) *_free_credits_out << bs->AvailableFor(v) << ',';
                if(_max_credits_out) *_max_credits_out << bs->LimitFor(v) << ',';
            }
        }
        for(int r = 0; r < _routers; ++r) {
            Router const * const rtr = _router[s][r];
            char trail_char =
                ((r == _routers - 1) && (s == _subnets - 1)) ? '\n' : ',';
            if(_used_credits_out) *_used_credits_out << rtr->UsedCredits() << trail_char;
            if(_free_credits_out) *_free_credits_out << rtr->FreeCredits() << trail_char;
            if(_max_credits_out) *_max_credits_out << rtr->MaxCredits() << trail_char;
        }
    }
    if(_used_credits_out) *_used_credits_out << flush;
    if(_free_credits_out) *_free_credits_out << flush;
    if(_max_credits_out) *_max_credits_out << flush;
#endif

}

void TrafficManager::DisplayStats(ostream & os) const {

  std::ofstream ofs_state;
  ofs_state.open("data/state");
  ofs_state<<"export var average_lat=[";
for(int c = 0; c < phase_num; ++c) {
    if(_flat_phase[c]>0)
  {ofs_state<<_flat_phase[c]->Average()<<",";}
  else {ofs_state<<0<<",";}
}
ofs_state<<"];"<<endl;

  ofs_state<<"export var max_lat=[";
for(int c = 0; c < phase_num; ++c) {
      if(_flat_phase[c]>0)
  {ofs_state<<_flat_phase[c]->Max()<<",";}
  else {ofs_state<<0<<",";}
}
ofs_state<<"];"<<endl;

  ofs_state<<"export var phase_cost=[";
for(int c = 0; c < phase_num; ++c) {
      if(_slowest_flit_phase[0][c]>0)
  {ofs_state<<phase_cost[c]<<",";}
  else {ofs_state<<0<<",";}
}
ofs_state<<"];"<<endl;

  ofs_state<<"export var slowest_flit_src=[";
for(int c = 0; c < phase_num; ++c) {
      if(_slowest_flit_phase[1][c]>0)
  {ofs_state<<_slowest_flit_phase[1][c]<<",";}
  else {ofs_state<<0<<",";}
}
ofs_state<<"];"<<endl;

  ofs_state<<"export var slowest_flit_dest=[";
for(int c = 0; c < phase_num; ++c) {
      if(_slowest_flit_phase[2][c]>0)
  {ofs_state<<_slowest_flit_phase[2][c]<<",";}
  else {ofs_state<<0<<",";}
}
ofs_state<<"];"<<endl;
  ofs_state<<"export var variance_lat=[";
for(int c = 0; c < phase_num; ++c) {
      if(_flat_phase[c]>0)
  {ofs_state<<_flat_phase[c]->Variance()<<",";}
  else {ofs_state<<0<<",";}
}
ofs_state<<"];"<<endl;
    for(int c = 0; c < _classes; ++c) {

        if(_measure_stats[c] == 0) {
            continue;
        }



        cout
            << "Packet latency average = " << _plat_stats[c]->Average() << endl
            << "\tminimum = " << _plat_stats[c]->Min() << endl
            << "\tmaximum = " << _plat_stats[c]->Max() << endl
            << "Network latency average = " << _nlat_stats[c]->Average() << endl
            << "\tminimum = " << _nlat_stats[c]->Min() << endl
            << "\tmaximum = " << _nlat_stats[c]->Max() << endl
            << "Slowest packet = " << _slowest_packet[c] << endl
            << "Flit latency average = " << _flat_stats[c]->Average() << endl
            << "\tminimum = " << _flat_stats[c]->Min() << endl
            << "\tmaximum = " << _flat_stats[c]->Max() << endl
            << "Slowest flit = " << _slowest_flit[c] << endl
            << "Fragmentation average = " << _frag_stats[c]->Average() << endl
            << "\tminimum = " << _frag_stats[c]->Min() << endl
            << "\tmaximum = " << _frag_stats[c]->Max() << endl;

        int count_sum, count_min, count_max;
        double rate_sum, rate_min, rate_max;
        double rate_avg;
        int sent_packets, sent_flits, accepted_packets, accepted_flits;
        int min_pos, max_pos;
        double time_delta = (double)(_time - _reset_time);
        _ComputeStats(_sent_packets[c], &count_sum, &count_min, &count_max, &min_pos, &max_pos);
        rate_sum = (double)count_sum / time_delta;
        rate_min = (double)count_min / time_delta;
        rate_max = (double)count_max / time_delta;
        rate_avg = rate_sum / (double)_nodes;
        sent_packets = count_sum;
        cout << "Injected packet rate average = " << rate_avg << endl
             << "\tminimum = " << rate_min
             << " (at node " << min_pos << ")" << endl
             << "\tmaximum = " << rate_max
             << " (at node " << max_pos << ")" << endl;
        _ComputeStats(_accepted_packets[c], &count_sum, &count_min, &count_max, &min_pos, &max_pos);
        rate_sum = (double)count_sum / time_delta;
        rate_min = (double)count_min / time_delta;
        rate_max = (double)count_max / time_delta;
        rate_avg = rate_sum / (double)_nodes;
        accepted_packets = count_sum;
        cout << "Accepted packet rate average = " << rate_avg << endl
             << "\tminimum = " << rate_min
             << " (at node " << min_pos << ")" << endl
             << "\tmaximum = " << rate_max
             << " (at node " << max_pos << ")" << endl;
        _ComputeStats(_sent_flits[c], &count_sum, &count_min, &count_max, &min_pos, &max_pos);
        rate_sum = (double)count_sum / time_delta;
        rate_min = (double)count_min / time_delta;
        rate_max = (double)count_max / time_delta;
        rate_avg = rate_sum / (double)_nodes;
        sent_flits = count_sum;
        cout << "Injected flit rate average = " << rate_avg << endl
             << "\tminimum = " << rate_min
             << " (at node " << min_pos << ")" << endl
             << "\tmaximum = " << rate_max
             << " (at node " << max_pos << ")" << endl;
        _ComputeStats(_accepted_flits[c], &count_sum, &count_min, &count_max, &min_pos, &max_pos);
        rate_sum = (double)count_sum / time_delta;
        rate_min = (double)count_min / time_delta;
        rate_max = (double)count_max / time_delta;
        rate_avg = rate_sum / (double)_nodes;
        accepted_flits = count_sum;
        cout << "Accepted flit rate average= " << rate_avg << endl
             << "\tminimum = " << rate_min
             << " (at node " << min_pos << ")" << endl
             << "\tmaximum = " << rate_max
             << " (at node " << max_pos << ")" << endl;

        cout << "Injected packet length average = " << (double)sent_flits / (double)sent_packets << endl
             << "Accepted packet length average = " << (double)accepted_flits / (double)accepted_packets << endl;

        cout << "Total in-flight flits = " << _total_in_flight_flits[c].size()
             << " (" << _measured_in_flight_flits[c].size() << " measured)"
             << endl;

#ifdef TRACK_STALLS
        _ComputeStats(_buffer_busy_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        os << "Buffer busy stall rate = " << rate_avg << endl;
        _ComputeStats(_buffer_conflict_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        os << "Buffer conflict stall rate = " << rate_avg << endl;
        _ComputeStats(_buffer_full_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        os << "Buffer full stall rate = " << rate_avg << endl;
        _ComputeStats(_buffer_reserved_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        os << "Buffer reserved stall rate = " << rate_avg << endl;
        _ComputeStats(_crossbar_conflict_stalls[c], &count_sum);
        rate_sum = (double)count_sum / time_delta;
        rate_avg = rate_sum / (double)(_subnets*_routers);
        os << "Crossbar conflict stall rate = " << rate_avg << endl;
#endif

    }
}

void TrafficManager::DisplayOverallStats( ostream & os ) const {

    os << "====== Overall Traffic Statistics ======" << endl;
    for ( int c = 0; c < _classes; ++c ) {

        if(_measure_stats[c] == 0) {
            continue;
        }

        os << "====== Traffic class " << c << " ======" << endl;

        os << "Packet latency average = " << _overall_avg_plat[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tminimum = " << _overall_min_plat[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tmaximum = " << _overall_max_plat[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;

        os << "Network latency average = " << _overall_avg_nlat[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tminimum = " << _overall_min_nlat[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tmaximum = " << _overall_max_nlat[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;

        os << "Flit latency average = " << _overall_avg_flat[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tminimum = " << _overall_min_flat[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tmaximum = " << _overall_max_flat[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;

        os << "Fragmentation average = " << _overall_avg_frag[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tminimum = " << _overall_min_frag[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tmaximum = " << _overall_max_frag[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;

        os << "Injected packet rate average = " << _overall_avg_sent_packets[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tminimum = " << _overall_min_sent_packets[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tmaximum = " << _overall_max_sent_packets[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;

        os << "Accepted packet rate average = " << _overall_avg_accepted_packets[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tminimum = " << _overall_min_accepted_packets[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tmaximum = " << _overall_max_accepted_packets[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;

        os << "Injected flit rate average = " << _overall_avg_sent[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tminimum = " << _overall_min_sent[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tmaximum = " << _overall_max_sent[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;

        os << "Accepted flit rate average = " << _overall_avg_accepted[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tminimum = " << _overall_min_accepted[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
        os << "\tmaximum = " << _overall_max_accepted[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;

        os << "Injected packet size average = " << _overall_avg_sent[c] / _overall_avg_sent_packets[c]
           << " (" << _total_sims << " samples)" << endl;

        os << "Accepted packet size average = " << _overall_avg_accepted[c] / _overall_avg_accepted_packets[c]
           << " (" << _total_sims << " samples)" << endl;

        os << "Hops average = " << _overall_hop_stats[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;

#ifdef TRACK_STALLS
        os << "Buffer busy stall rate = " << (double)_overall_buffer_busy_stalls[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl
           << "Buffer conflict stall rate = " << (double)_overall_buffer_conflict_stalls[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl
           << "Buffer full stall rate = " << (double)_overall_buffer_full_stalls[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl
           << "Buffer reserved stall rate = " << (double)_overall_buffer_reserved_stalls[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl
           << "Crossbar conflict stall rate = " << (double)_overall_crossbar_conflict_stalls[c] / (double)_total_sims
           << " (" << _total_sims << " samples)" << endl;
#endif

    }

}

string TrafficManager::_OverallStatsCSV(int c) const
{
    ostringstream os;
    os << _traffic[c]
       << ',' << _use_read_write[c]
       << ',' << _load[c]
       << ',' << _overall_min_plat[c] / (double)_total_sims
       << ',' << _overall_avg_plat[c] / (double)_total_sims
       << ',' << _overall_max_plat[c] / (double)_total_sims
       << ',' << _overall_min_nlat[c] / (double)_total_sims
       << ',' << _overall_avg_nlat[c] / (double)_total_sims
       << ',' << _overall_max_nlat[c] / (double)_total_sims
       << ',' << _overall_min_flat[c] / (double)_total_sims
       << ',' << _overall_avg_flat[c] / (double)_total_sims
       << ',' << _overall_max_flat[c] / (double)_total_sims
       << ',' << _overall_min_frag[c] / (double)_total_sims
       << ',' << _overall_avg_frag[c] / (double)_total_sims
       << ',' << _overall_max_frag[c] / (double)_total_sims
       << ',' << _overall_min_sent_packets[c] / (double)_total_sims
       << ',' << _overall_avg_sent_packets[c] / (double)_total_sims
       << ',' << _overall_max_sent_packets[c] / (double)_total_sims
       << ',' << _overall_min_accepted_packets[c] / (double)_total_sims
       << ',' << _overall_avg_accepted_packets[c] / (double)_total_sims
       << ',' << _overall_max_accepted_packets[c] / (double)_total_sims
       << ',' << _overall_min_sent[c] / (double)_total_sims
       << ',' << _overall_avg_sent[c] / (double)_total_sims
       << ',' << _overall_max_sent[c] / (double)_total_sims
       << ',' << _overall_min_accepted[c] / (double)_total_sims
       << ',' << _overall_avg_accepted[c] / (double)_total_sims
       << ',' << _overall_max_accepted[c] / (double)_total_sims
       << ',' << _overall_avg_sent[c] / _overall_avg_sent_packets[c]
       << ',' << _overall_avg_accepted[c] / _overall_avg_accepted_packets[c]
       << ',' << _overall_hop_stats[c] / (double)_total_sims;

#ifdef TRACK_STALLS
    os << ',' << (double)_overall_buffer_busy_stalls[c] / (double)_total_sims
       << ',' << (double)_overall_buffer_conflict_stalls[c] / (double)_total_sims
       << ',' << (double)_overall_buffer_full_stalls[c] / (double)_total_sims
       << ',' << (double)_overall_buffer_reserved_stalls[c] / (double)_total_sims
       << ',' << (double)_overall_crossbar_conflict_stalls[c] / (double)_total_sims;
#endif

    return os.str();
}

void TrafficManager::DisplayOverallStatsCSV(ostream & os) const {
    for(int c = 0; c < _classes; ++c) {
        os << "results:" << c << ',' << _OverallStatsCSV() << endl;
    }
}

//read the watchlist
void TrafficManager::_LoadWatchList(const string & filename){
    ifstream watch_list;
    watch_list.open(filename.c_str());

    string line;
    if(watch_list.is_open()) {
        while(!watch_list.eof()) {
            getline(watch_list, line);
            if(line != "") {
                if(line[0] == 'p') {
                    _packets_to_watch.insert(atoi(line.c_str()+1));
                } else {
                    _flits_to_watch.insert(atoi(line.c_str()));
                }
            }
        }

    } else {
        Error("Unable to open flit watch file: " + filename);
    }
}

int TrafficManager::_GetNextPacketSize(int cl) const
{
    assert(cl >= 0 && cl < _classes);

    vector<int> const & psize = _packet_size[cl];
    int sizes = psize.size();

    if(sizes == 1) {
        return psize[0];
    }

    vector<int> const & prate = _packet_size_rate[cl];
    int max_val = _packet_size_max_val[cl];

    int pct = RandomInt(max_val);

    for(int i = 0; i < (sizes - 1); ++i) {
        int const limit = prate[i];
        if(limit > pct) {
            return psize[i];
        } else {
            pct -= limit;
        }
    }
    assert(prate.back() > pct);
    return psize.back();
}

double TrafficManager::_GetAveragePacketSize(int cl) const
{
    vector<int> const & psize = _packet_size[cl];
    int sizes = psize.size();
    if(sizes == 1) {
        return (double)psize[0];
    }
    vector<int> const & prate = _packet_size_rate[cl];
    int sum = 0;
    for(int i = 0; i < sizes; ++i) {
        sum += psize[i] * prate[i];
    }
    return (double)sum / (double)(_packet_size_max_val[cl] + 1);
}
