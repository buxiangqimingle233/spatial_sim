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

/*flit.cpp
 *
 *flit struct is a flit, carries all the control signals that a flit needs
 *Add additional signals as necessary. Flits has no concept of length
 *it is a singluar object.
 *
 *When adding objects make sure to set a default value in this constructor
 */

#include "booksim.hpp"
#include "globals.hpp"
#include "flit.hpp"

stack<Flit *> Flit::_all;
stack<Flit *> Flit::_free;

ostream& operator<<( ostream& os, const Flit& f )
{
  os << "  Flit ID: " << f.id << " (" << &f << ")" 
     << " Packet ID: " << f.pid
     << " Type: " << f.type 
     << " Head: " << f.head
     << " Tail: " << f.tail << endl;
  os << "  Source: " << f.src << "  Dest: " << f.dest << " Intm: "<<f.intm<<endl;
  os << "  Creation time: " << f.ctime << " Injection time: " << f.itime << " Arrival time: " << f.atime << " Phase: "<<f.ph<< endl;
  os << "  VC: " << f.vc << endl;
  return os;
}

Flit::Flit() 
{  
  Reset();
}  

void Flit::Reset() 
{  
  type      = ANY_TYPE ;
  vc        = -1 ;
  cl        = -1 ;
  head      = false ;
  tail      = false ;
  ctime     = -1 ;
  itime     = -1 ;
  atime     = -1 ;
  id        = -1 ;
  pid       = -1 ;
  hops      = 0 ;
  watch     = false ;
  record    = false ;
  intm = 0;
  src = -1;
  dest = -1;
  pri = 0;
  intm =-1;
  ph = -1;
  data = 0;
}

Flit *Flit::New() {
  return new FocusFlit();
}

// Flit * Flit::New() {
//   Flit * f;
//   if(_free.empty()) {
//     f = new FocusFlit();
//     // f = new Flit;
//     // _all.push(f);
//   } else {
//     f = _free.top();
//     f->Reset();
//     _free.pop();
//   }
//   return f;
// }

void Flit::Free() {
  _free.push(this);
}

void Flit::FreeAll() {
  while(!_all.empty()) {
    delete _all.top();
    _all.pop();
  }
}

void Flit::GC() {
  while (!_free.empty()) {
    delete _free.top();
    _free.pop();
  }
}

void FocusFlit::Reset() {
  Flit::Reset();
  head_flit = nullptr;
  pkt = spatial::Packet();
  seg = Segment(make_pair(-1, -1));
  path = nullptr;
  while (im_nodes.empty()) {
    im_nodes.pop();
  }
  childs.clear();
}

vector<int> FocusFlit::GetFanoutFlitTargets(int rid) {
  std::vector<int> targets;
  // This flit reach the end
  if (seg.second == rid) {
    if (path->isDestNode(rid)) {
      targets.push_back(rid);
    }
    for (auto f: childs) {
      std::vector<int> child_first_hop = f->GetFanoutFlitTargets(rid);
      assert(child_first_hop.size() == 1);  // splitted flits mush take at least one unicast step after initialized
      targets.push_back(child_first_hop[0]);
    }
  } else {
    while (!im_nodes.empty() && im_nodes.front() == rid) {
      im_nodes.pop();
    }
    if (im_nodes.empty()) {
      targets.push_back(seg.second);
    } else {
      targets.push_back(im_nodes.front());
    }
  }
  return targets;
}

vector<FocusFlit*> FocusFlit::GetFanoutFlits(int rid) {
  int time = GetSimTime();
  vector<FocusFlit*> ret;
  if (seg.second != rid) {   // On the way to segment destination
    ret.push_back(this);
  } else {    // We have reached the segment destination
    if (path->isDestNode(seg.second)) {
      ret.push_back(this);
    }
    for (FocusFlit* f: childs) {
      ret.push_back(f);
      f->ctime = time;
    }
  }
  return ret;
}

int FocusFlit::GetFanoutFlitNum(int rid) {
  if (seg.second == rid) {
    return childs.size() + (path->isDestNode(rid) ? 1 : 0);
  } else {
    return 1;
  }
}

bool FocusFlit::IsRetired(int rid) {
  return im_nodes.empty() && rid == seg.second;
}

bool FocusFlit::IsRealFlit() {
  return path->isDestNode(seg.second); 
}

int FocusFlit::SegEnd() {
  return seg.second;
}

int FocusFlit::SegStart() {
  return seg.first;
}

FocusFlit* FocusFlit::NewFlitTree(shared_ptr<MCTree> path, Segment seg) {
  assert(seg.first != seg.second);
  // Source
  FocusFlit* flit = static_cast<FocusFlit*>(Flit::New());
  
  flit->seg = seg;
  flit->path = path;
  flit->im_nodes = *(path->intermediateNodes(seg));
  
  vector<Segment> succeeds = path->segmentStartedWith(seg.second);
  for (Segment s: succeeds) {
    flit->childs.push_back(NewFlitTree(path, s));
  }
  return flit;
}

vector<FocusFlit*> FocusFlit::GetChilds() {
  return childs;
}


void FocusFlit::SyncWithHeadFlit(FocusFlit* header) {
  assert(header->head);
  assert(header->seg == this->seg);
  head_flit = header;

  assert(header->childs.size() == this->childs.size());
  for (int i = 0; i < childs.size(); ++i) {
    childs[i]->SyncWithHeadFlit(header->childs[i]);
  }
}