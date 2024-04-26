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

// ----------------------------------------------------------------------
//
//  File Name: flitchannel.cpp
//  Author: James Balfour, Rebecca Schultz
//
// ----------------------------------------------------------------------

#include "flitchannel.hpp"

#include <iostream>
#include <iomanip>

#include "router.hpp"
#include "globals.hpp"

// ----------------------------------------------------------------------
//  $Author: jbalfour $
//  $Date: 2007/06/27 23:10:17 $
//  $Id$
// ----------------------------------------------------------------------
FlitChannel::FlitChannel(Module * parent, string const & name, int classes)
: Channel<Flit>(parent, name), _routerSource(NULL), _routerSourcePort(-1),
  _routerSink(NULL), _routerSinkPort(-1), _idle(0) {
    id=-1;
  _active.resize(classes, 0);
}

void FlitChannel::SetSource(Router const * const router, int port) {
  _routerSource = router;
  _routerSourcePort = port;
}

void FlitChannel::SetSink(Router const * const router, int port) {
  _routerSink = router;
  _routerSinkPort = port;
}

void FlitChannel::Send(Flit * f) {
  if(f) {
    ++_active[f->cl];
  } else {
    ++_idle;
  }
  Channel<Flit>::Send(f);
}

void FlitChannel::ReadInputs() {
  Flit const * const & f = _input;
  if(f && f->watch) {
    *gWatchOut << GetSimTime() << " | " << FullName() << " | "
	       << "Beginning channel traversal for flit " << f->id
	       << " with delay " << _delay
	       << "." << endl;
  }
  Channel<Flit>::ReadInputs();
}

void FlitChannel::WriteOutputs() {
  Channel<Flit>::WriteOutputs();
  if(_output && _output->watch) {
    *gWatchOut << GetSimTime() << " | " << FullName() << " | "
	       << "Completed channel traversal for flit " << _output->id
	       << "." << endl;
  }

  if(_output&&id!=-1)
  {
    step_occupation[GetSimTime()/measure_step][id]++; //modifyyyyyy

       map<int ,int > ::iterator current_group;
        current_group = phase_group.find(_output->src);
        // cout<<current_group->second<<endl;
        occupation[current_group->second][id]++;
    // if(id==284)
    // {
    //   cout<<GetSimTime()<<"|"<<id<<" "<<_output->src<<"->"<<_output->dest<<" "<<endl;
    // }
    // if(id==4058)
    // cout<<GetSimTime()<<"|"<<id<<" "<<_output->src<<"->"<<_output->dest<<" "<<_output->phase<<endl;

    // cout<<id<<" "<<step_occupation[GetSimTime()/1000][id]<<endl;

  //         current_channel = channel_occupation.find(id);
	// 			if(current_channel!=channel_occupation.end())
  //       {
  //         current_channel->second++;
  //         cout<<current_connection->second.first<<" to "<<current_connection->second.second<<" "<<current_channel->second<<endl;
  //       }
  //       else{channel_occupation.insert(pair<int,int>(id,1));  }
        }

}
