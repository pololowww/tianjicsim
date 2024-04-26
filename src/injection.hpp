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

#ifndef _INJECTION_HPP_
#define _INJECTION_HPP_

#include "config_utils.hpp"

using namespace std;

class InjectionProcess {
protected:
  int _nodes;
  double _rate;
  vector<vector<int>> _lut;
  InjectionProcess(int nodes, double rate,vector<vector<int>>& lut);
public:
  virtual ~InjectionProcess() {}
  virtual bool test(int source,vector<vector<int>>& lut,int run_time,int period) = 0;
  virtual void reset();
  static InjectionProcess * New(string const & inject, int nodes, double load, vector<vector<int>>& lut,
				Configuration const * const config = NULL);
};

class BernoulliInjectionProcess : public InjectionProcess {
public:
  BernoulliInjectionProcess(int nodes, double rate,vector<vector<int>>& lut);
  virtual bool test(int source,vector<vector<int>>& lut,int run_time,int period);
};

class OnOffInjectionProcess : public InjectionProcess {
private:
  double _alpha;
  double _beta;
  double _r1;
  vector<int> _initial;
  vector<int> _state;
public:
  OnOffInjectionProcess(int nodes, double rate, double alpha, double beta, 
			vector<vector<int>>& lut, double r1,vector<int> initial);
  virtual void reset();
  virtual bool test(int source,vector<vector<int>>& lut,int run_time,int period);
};

#endif 