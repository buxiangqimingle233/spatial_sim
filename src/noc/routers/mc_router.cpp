/**
 * @file mc_router.cpp
 * @author wangzhao
 * @brief Input queued router with multicast supports. We assume no speculative,
 *  no noq, and no _hold_switch_for_packet, routing delay must equal to 1
 * @date 2022-03-29
 *
 * @copyright Copyright (c) 2022
 *
 */


#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <deque>
#include <assert.h>
#include <stdlib.h>
#include <limits>
#include <random>
#include <algorithm>

#include "flit.hpp"
#include "multi_output.hpp"
#include "buffer_state.hpp"
#include "buffer_monitor.hpp"
#include "switch_monitor.hpp"
#include "mc_router.hpp"


void MCRouter::_InternalStep() {
    assert(!_noq && !_speculative && !_hold_switch_for_packet);

    // BufferState* debug_2 = _next_buf[2];
    // MCBuffer* in_4 = _mc_buf[4];
    // if (GetID() == 0) {
    //     debug_2->Display();
    //     in_4->Display();
    // }

    if (!_active) {
        return;
    }

    _FlitDispatch();
    bool activity = !_proc_credits.empty();

    // Multicast control pane goes first
    if (!_multi_route_vcs.empty()) {
        _MultiRouteEvaluate();
    }
    assert(_multi_vc_allocator);
    _multi_vc_allocator->Clear();
    if (!_multi_vc_alloc_vcs.empty()) {
        _MultiVCAllocEvaluate();
    }
    // We haven't implement it yet ...
    assert(!_hold_switch_for_packet);
    assert(_multi_sw_allocator);
    _multi_sw_allocator->Clear();
    if (!_multi_sw_alloc_vcs.empty()) {
        _MultiSWAllocEvaluate();
    }

    // To allocate switch to multicast packets first, we update SW allocation before evaluate the swith.
    // But it has the side-effect: it updates switch pending packets, the multicast flit will go through
    // the switch allocation phase and the switch transfer phase within one cycle. The 4-cycle pipeline
    // is thus shortened into 3 cycles.

    if (!_multi_route_vcs.empty()) {
        _MultiRouteUpdate();
        activity = activity || !_multi_route_vcs.empty();
    }
    if (!_multi_vc_alloc_vcs.empty()) {
        _MultiVCAllocUpdate();
        activity = activity || !_multi_vc_alloc_vcs.empty();
    }
    if (!_multi_sw_alloc_vcs.empty()) {
        _MultiSWAllocUpdate();
        activity = activity || !_multi_sw_alloc_vcs.empty();
    }


    // Unicast control pane
    if (!_route_vcs.empty())
        _RouteEvaluate();
    if (_vc_allocator) {
        _vc_allocator->Clear();
        if (!_vc_alloc_vcs.empty()) {
            _VCAllocEvaluate();
        }
    }
    if (_hold_switch_for_packet)
    {
        if (!_sw_hold_vcs.empty())
            _SWHoldEvaluate();
    }
    _sw_allocator->Clear();
    if (_spec_sw_allocator)
        _spec_sw_allocator->Clear();
    if (!_sw_alloc_vcs.empty())
        _SWAllocEvaluate();

    // Share switch
    if (!_crossbar_flits.empty())
        _SwitchEvaluate();

    // Update below
    if (!_route_vcs.empty()) {
        _RouteUpdate();
        activity = activity || !_route_vcs.empty();
    }
    if (!_vc_alloc_vcs.empty()) {
        _VCAllocUpdate();
        activity = activity || !_vc_alloc_vcs.empty();
    }
    if (_hold_switch_for_packet) {
        if (!_sw_hold_vcs.empty()) {
            _SWHoldUpdate();
            activity = activity || !_sw_hold_vcs.empty();
        }
    }
    if (!_sw_alloc_vcs.empty()) {
        _SWAllocUpdate();
        activity = activity || !_sw_alloc_vcs.empty();
    }

    // Multicast & Unicast share the crossbar manipulation procedure
    if (!_crossbar_flits.empty()) {
        _SwitchUpdate();
        activity = activity || !_crossbar_flits.empty();
    }
    _OutputQueuing();

    _active = activity;


    _bufferMonitor->cycle();
    _switchMonitor->cycle();


}


void MCRouter::_FlitDispatch() {
    for (map<int, Flit* >::const_iterator iter = _in_queue_flits.begin();
         iter != _in_queue_flits.end();
         ++iter)
    {
        int const input = iter->first;
        assert((input >= 0) && (input < _inputs));

        Flit *const f = iter->second;
        assert(f);

        int const vc = f->vc;
        assert((vc >= 0) && (vc < _vcs));

        bool multicast = static_cast<FocusFlit*>(f)->GetFanoutFlitNum(GetID()) > 1;
        multicast = true;   // FIXME: All flits goes to multicast data pane

        Buffer* cur_buf;

        if (multicast) {
            cur_buf = static_cast<Buffer*>(_mc_buf[input]);
        } else {
            cur_buf = _buf[input];
        }

        if (f->watch) {
            *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                       << "Adding flit " << f->id
                       << " to VC " << vc
                       << " at input " << input
                       << " with ";
            if (multicast) {
                *gWatchOut << "multicast support";
            }
            *gWatchOut << " (state: " << VC::VCSTATE[cur_buf->GetState(vc)];
            if (cur_buf->Empty(vc)) {
                *gWatchOut << ", empty";
            } else {
                assert(cur_buf->FrontFlit(vc));
                *gWatchOut << ", front: " << cur_buf->FrontFlit(vc)->id;
            }
            *gWatchOut << ").";
            if (multicast) {
                *gWatchOut << " [Multicast Flit].";
            }
            *gWatchOut << endl;
        }
        cur_buf->AddFlit(vc, f);

        // We keep this although not clear with its effects
        _bufferMonitor->write(input, f);

        if (cur_buf->GetState(vc) == VC::idle) {
            assert(cur_buf->FrontFlit(vc) == f);
            assert(cur_buf->GetOccupancy(vc) == 1);
            assert(f->head);
            assert(_routing_delay == 1);

            cur_buf->SetState(vc, VC::routing);
            if (multicast) {
                _multi_route_vcs.push_back(make_pair(-1, make_pair(input, vc)));
            } else {
                _route_vcs.push_back(make_pair(-1, make_pair(input, vc)));
            }
        }
        else if ((cur_buf->GetState(vc) == VC::active)
              && (cur_buf->FrontFlit(vc) == f))
        {
            if (multicast) {
                _multi_sw_alloc_vcs.push_back(make_pair(-1, make_pair(make_pair(input, vc), vector<int>())));
            } else {
                _sw_alloc_vcs.push_back(make_pair(-1, make_pair(make_pair(input, vc), -1)));
            }
        }
    }

    _in_queue_flits.clear();

    while (!_proc_credits.empty()) {
        pair<int, pair<Credit*, int> > const &item = _proc_credits.front();

        // _proc_credits are inserted in the time descending order
        int const time = item.first;
        if (GetSimTime() < time) {
            break;
        }

        Credit *const c = item.second.first;
        assert(c);
        int const output = item.second.second;
        assert((output >= 0) && (output < _outputs));

        BufferState *const dest_buf = _next_buf[output];

        dest_buf->ProcessCredit(c);
        c->Free();
        _proc_credits.pop_front();
    }
}


void MCRouter::_MultiRouteEvaluate() {
    assert(_routing_delay == 1);

    for (deque<pair<int, pair<int, int>>>::iterator iter = _multi_route_vcs.begin();
         iter != _multi_route_vcs.end();
         iter++)
    {
        int const time = iter->first;
        if (time >= 0) {
            break;
        }

        iter->first = GetSimTime() + _routing_delay - 1;

        int const input = iter->second.first;
        assert((input >= 0) && (input < _inputs));
        int const vc = iter->second.second;
        assert((vc >= 0) && (vc < _vcs));

        MCBuffer const *const cur_buf = _mc_buf[input];
        assert(!cur_buf->Empty(vc));
        assert(cur_buf->GetState(vc) == VC::routing);

        FocusFlit* f = static_cast<FocusFlit*>(cur_buf->FrontFlit(vc));
        assert(f);
        assert(f->vc == vc);
        assert(f->head);
        assert(f->GetFanoutFlitNum(GetID()) >= 1);

        if (f->watch)
        {
            *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                        << "Beginning routing for VC " << vc
                        << " at input " << input
                        << " with multicast support"
                        << " (front: " << f->id
                        << ").  [Multicast Flit]" << endl;
        }
    }
}


void MCRouter::_MultiRouteUpdate() {
    assert(_routing_delay == 1);

    while (!_multi_route_vcs.empty()) {
        auto& item = _multi_route_vcs.front();

        int const time = item.first;
        if ((time < 0) || (GetSimTime() < time)) {
            break;
        }
        assert(GetSimTime() == time);

        int const input = item.second.first;
        assert((input >= 0) && (input < _inputs));
        int const vc = item.second.second;
        assert((vc >= 0) && (vc < _vcs));

        MCBuffer *const cur_buf = _mc_buf[input];
        assert(!cur_buf->Empty(vc));
        assert(cur_buf->GetState(vc) == VC::routing);

        FocusFlit *f = static_cast<FocusFlit*>(cur_buf->FrontFlit(vc));
        assert(f);
        assert(f->vc == vc);
        assert(f->head);

        if (f->watch)
        {
            *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                        << "Completed routing for VC " << vc
                        << " at input " << input
                        << " (front: " << f->id
                        << ").  [Multicast Flit]" << endl;
        }

        cur_buf->Route(vc, _rf, this, f, input);
        cur_buf->SetState(vc, VC::vc_alloc);

        if (_vc_allocator) {
            _multi_vc_alloc_vcs.push_back(make_pair(-1, make_pair(item.second, vector<int>())));
        }
        _multi_route_vcs.pop_front();
    }
}


void MCRouter::_MultiVCAllocEvaluate() {
    assert(_vc_allocator);

    bool watched = false;

    for (auto iter = _multi_vc_alloc_vcs.begin(); iter != _multi_vc_alloc_vcs.end(); ++iter)
    {

        int const time = iter->first;
        if (time >= 0) {
            break;
        }

        int const input = iter->second.first.first;
        assert((input >= 0) && (input < _inputs));
        int const vc = iter->second.first.second;
        assert((vc >= 0) && (vc < _vcs));

        assert(iter->second.second.empty());

        MCBuffer const *const cur_buf = _mc_buf[input];
        assert(!cur_buf->Empty(vc));
        assert(cur_buf->GetState(vc) == VC::vc_alloc);

        FocusFlit *const f = static_cast<FocusFlit*>(cur_buf->FrontFlit(vc));
        assert(f);
        assert(f->vc == vc);
        assert(f->head);


        watched |= f->watch;

        if (f->watch) {
            *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                        << "Beginning VC allocation for VC " << vc
                        << " at input " << input
                        << " (front: " << f->id
                        << ") [Multicast Flit]." << endl;
        }

        OutputSet const *const route_set = cur_buf->GetRouteSet(vc);
        assert(route_set);

        int const out_priority = cur_buf->GetPriority(vc);
        set<OutputSet::sSetElement> const setlist = route_set->GetSet();

        bool elig = true;
        bool reserved = false;

        // assert(!_noq || (setlist.size() >= 1));
        assert(!_noq && (setlist.size() == f->GetFanoutFlitNum(GetID())));

        set<int> vis;
        // These requests are buffered here. They are submitted to the allocator
        // only if all output ports are available.

        struct Paras {
            int in, out, label, in_pri, out_pri;
        };

        std::vector<Paras> req_paras;
        for (auto iset = setlist.begin(); iset != setlist.end(); ++iset)
        {

            int const out_port = iset->output_port;
            assert((out_port >= 0) && (out_port < _outputs));

            // The routing result must have different output ports
            assert(vis.find(out_port) == vis.end());
            vis.insert(out_port);

            BufferState const *const dest_buf = _next_buf[out_port];

            int vc_start = iset->vc_start, vc_end = iset->vc_end;
            assert(vc_start >= 0 && vc_start < _vcs);
            assert(vc_end >= 0 && vc_end < _vcs);
            assert(vc_end >= vc_start);
            assert(iset->target >= 0);

            // We randomly select an output virtual channel for multicast packets
            int out_vc = 0;
            if (vc_end != 0) {
                out_vc = rand() % (vc_end - vc_start + 1) + vc_start;
            }

            assert((out_vc >= 0) && (out_vc < _vcs));

            int in_priority = iset->pri;
            if (_vc_prioritize_empty && !dest_buf->IsEmptyFor(out_vc))
            {
                assert(in_priority >= 0);
                in_priority += numeric_limits<int>::min();
            }

            if (!dest_buf->IsAvailableFor(out_vc)) {
                if (f->watch) {
                    int const use_input_and_vc = dest_buf->UsedBy(out_vc);
                    int const use_input = use_input_and_vc / _vcs;
                    int const use_vc = use_input_and_vc % _vcs;
                    *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                            << "  VC " << out_vc
                            << " at output " << out_port
                            << " is in use by VC " << use_vc
                            << " at input " << use_input;
                    Flit *cf = _mc_buf[use_input]->FrontFlit(use_vc);
                    if (cf) {
                        *gWatchOut << " (front flit: " << cf->id << ")";
                    } else {
                        *gWatchOut << " (empty)";
                    }
                    *gWatchOut << "." << endl;
                }
                elig = false;   // A multicast flit needs all output ports available
            } else {
                int const input_and_vc = _vc_shuffle_requests ? (vc * _inputs + input) : (input * _vcs + vc);
                req_paras.push_back({input_and_vc, out_port * _vcs + out_vc, 0, in_priority, out_priority});
            }
        }

        if (!elig) {            // Some output ports are unavailabe
            iter->second.second.push_back(STALL_BUFFER_BUSY);
        } else {    // Submit the requests to the allocator
            for (auto p: req_paras) {
               _multi_vc_allocator->AddRequest(p.in, p.out, p.label, p.in_pri, p.out_pri);
            }
        }
    }

    if (watched) {
        *gWatchOut << GetSimTime() << " | " << _multi_vc_allocator->FullName() << " | ";
        _multi_vc_allocator->PrintRequests(gWatchOut);
    }

    _multi_vc_allocator->Allocate();

    if (watched) {
        *gWatchOut << GetSimTime() << " | " << _multi_vc_allocator->FullName() << " | ";
        _multi_vc_allocator->PrintRequests(gWatchOut);
    }

    for (auto iter = _multi_vc_alloc_vcs.begin(); iter != _multi_vc_alloc_vcs.end(); ++iter) {
        int const time = iter->first;
        if (time >= 0) {
            break;
        }

        iter->first = GetSimTime() + _vc_alloc_delay - 1;

        int const input = iter->second.first.first;
        assert((input >= 0) && (input < _inputs));
        int const vc = iter->second.first.second;
        assert((vc >= 0) && (vc < _vcs));

        if (!iter->second.second.empty()) {
            assert(iter->second.second.front() < -1);
            continue;
        }

        MCBuffer *const cur_buf = _mc_buf[input];
        assert(!cur_buf->Empty(vc));
        assert(cur_buf->GetState(vc) == VC::vc_alloc);

        FocusFlit *const f = static_cast<FocusFlit*>(cur_buf->FrontFlit(vc));
        assert(f);
        assert(f->vc == vc);
        assert(f->head);

        int const input_and_vc = _vc_shuffle_requests ? (vc * _inputs + input) : (input * _vcs + vc);
        vector<int> outputs_and_vcs = _multi_vc_allocator->MultiOutputAssigned(input_and_vc);

        assert(outputs_and_vcs.size() == f->GetFanoutFlitNum(GetID()));

        bool grant_succeed = true;
        for (int it: outputs_and_vcs) {
            grant_succeed &= it >= 0;
        }

        if (grant_succeed) {
            if (f->watch) {
                *gWatchOut << GetSimTime() << " | " << FullName() << " | " << "Assigning VC ";
                for (int output_and_vc: outputs_and_vcs) {
                    int const match_output = output_and_vc / _vcs;
                    assert((match_output >= 0) && (match_output < _outputs));
                    int const match_vc = output_and_vc % _vcs;
                    assert((match_vc >= 0) && (match_vc < _vcs));
                    *gWatchOut << match_vc << " at output " << match_output << " ; ";
                }
                *gWatchOut << " to VC " << vc << " at input " << input << ". [Multicast Flit]" << endl;
            }
            iter->second.second = outputs_and_vcs;

            // Multicast flits grab the buffer before vc_allocate_update. Although it violates
            // the two-phase update scheme, it forbids the unicast flits from being allocated to
            // the output channels that multicast flits have already grabbed.

            // clean the out_pair
            cur_buf->ClearOutputs(vc);

            for (int output_and_vc: outputs_and_vcs) {
                int const match_output = output_and_vc / _vcs;
                int const match_vc = output_and_vc % _vcs;
                assert((match_output >= 0) && (match_output < _outputs));
                assert((match_vc >= 0) && (match_vc < _vcs));

                if (f->watch)
                {
                    *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                            << "  Acquiring assigned VC " << match_vc
                            << " at output " << match_output
                            << ". [Multicast Flit] " << endl;
                }

                BufferState *const dest_buf = _next_buf[match_output];
                assert(dest_buf->IsAvailableFor(match_vc));

                dest_buf->TakeBuffer(match_vc, input * _vcs + vc);
                cur_buf->addMultiOutput(vc, match_output, match_vc);
            }

        } else {
            if (f->watch)
            {
                *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                        << "VC allocation failed for VC " << vc
                        << " at input " << input
                        << ". [Multicast Flit]" << endl;
            }
            iter->second.second.push_back(STALL_BUFFER_CONFLICT);
        }
    }

    assert(_vc_alloc_delay == 1);

}


void MCRouter::_MultiVCAllocUpdate() {
    assert(_vc_allocator);

    while (!_multi_vc_alloc_vcs.empty()) {
        auto& item = _multi_vc_alloc_vcs.front();

        int const time = item.first;

        if ((time < 0) || (GetSimTime() < time)) {
            break;
        }
        assert(GetSimTime() == time);

        int const input = item.second.first.first;
        assert((input >= 0) && (input < _inputs));
        int const vc = item.second.first.second;
        assert((vc >= 0) && (vc < _vcs));

        assert(!item.second.second.empty());

        MCBuffer *const cur_buf = _mc_buf[input];
        assert(!cur_buf->Empty(vc));
        assert(cur_buf->GetState(vc) == VC::vc_alloc);

        FocusFlit *const f = static_cast<FocusFlit*>(cur_buf->FrontFlit(vc));
        assert(f);
        assert(f->vc == vc);
        assert(f->head);
        assert(f->GetFanoutFlitNum(GetID()) >= 1);  // FIXME:

        if (f->watch)
        {
            *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                        << "Completed VC allocation for VC " << vc
                        << " at input " << input
                        << " (front: " << f->id
                        << "). [Multicast Flit]" << endl;
        }

        vector<int> outputs_and_vcs = item.second.second;
        assert(!outputs_and_vcs.empty());

        bool grant_succeed = true;
        for (int it: outputs_and_vcs) {
            grant_succeed &= it >= 0;
        }

        if (grant_succeed) {
            cur_buf->SetState(vc, VC::active);
            _multi_sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, vector<int>())));
        } else {
            if (f->watch) {
                *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                           << "  Not all required VCs are allocated. [Multicast Flit]" << endl;
            }
            _multi_vc_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, vector<int>())));
        }
        _multi_vc_alloc_vcs.pop_front();
    }

}


void MCRouter::_MultiSWAllocEvaluate() {

    bool watched = false;

    for (auto iter = _multi_sw_alloc_vcs.begin(); iter != _multi_sw_alloc_vcs.end(); ++iter) {
        int const time = iter->first;
        if (time >= 0) {
            break;
        }

        int const input = iter->second.first.first;
        assert((input >= 0) && (input < _inputs));
        int const vc = iter->second.first.second;
        assert((vc >= 0) && (vc < _vcs));

        assert(iter->second.second.empty());

        MCBuffer *const cur_buf = _mc_buf[input];
        assert(!cur_buf->Empty(vc));
        assert(cur_buf->GetState(vc) == VC::active);

        FocusFlit *const f = static_cast<FocusFlit*>(cur_buf->FrontFlit(vc));
        assert(f);
        assert(f->vc == vc);
        assert(f->GetFanoutFlitNum(GetID()));

        watched |= f->watch;

        if (f->watch)
        {
            *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                        << "Beginning switch allocation for VC " << vc
                        << " at input " << input
                        << " (front: " << f->id
                        << "). [Multicast Flit]" << endl;
        }

        if (cur_buf->GetState(vc) == VC::active) {
            std::map<int, int> outputs = cur_buf->getMultiOutputs(vc);

            bool elig = true;

            struct Paras {
                int in, out, label, in_pri, out_pri;
            };

            std::vector<Paras> req_paras;
            for (auto out_iter = outputs.begin(); out_iter != outputs.end(); ++out_iter) {
                int dest_output = out_iter->first;
                assert((dest_output >= 0) && (dest_output < _outputs));
                int dest_vc = out_iter->second;
                assert((dest_vc >= 0) && (dest_vc < _vcs));

                BufferState const *const dest_buf = _next_buf[dest_output];

                if (dest_buf->IsFullFor(dest_vc)) {
                    if (f->watch)
                    {
                        *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                    << "  VC " << dest_vc
                                    << " at output " << dest_output
                                    << " is full. [Multicast Flit]" << endl;
                    }
                    elig = false;
                    break;
                } else {
                    int prio = cur_buf->GetPriority(vc);
                    // FIXME: We change the input encoding scheme to input * vcs + vc.
                    // It means that we just support input_speedup and output_speedup of 1.
                    assert(_input_speedup == 1 && _output_speedup == 1);
                    // int const expanded_input = input * _input_speedup + vc % _input_speedup;
                    // int const expanded_output = dest_output * _output_speedup + input % _output_speedup;
                    int const expanded_input = input * _vcs + vc;
                    int const expanded_output = dest_output;
                    req_paras.push_back({expanded_input, expanded_output, vc, prio, prio});
                }
            }

            if (elig) {
                watched |= f->watch;

                // TODO: refine this according to _SWAllocAddReq
                for (auto p: req_paras) {
                    _multi_sw_allocator->AddRequest(p.in, p.out, p.label, p.in_pri, p.out_pri);
                }

            } else {
                // What about STALL_BUFFER_RESERVED ?
                iter->second.second.push_back(STALL_BUFFER_FULL);
                continue;
            }
        }
    }

    if (watched) {
        *gWatchOut << GetSimTime() << " | " << _multi_sw_allocator->FullName() << " | ";
        _multi_sw_allocator->PrintRequests(gWatchOut);
    }

    _multi_sw_allocator->Allocate();

    if (watched) {
        *gWatchOut << GetSimTime() << " | " << _multi_sw_allocator->FullName() << " | ";
        _multi_sw_allocator->PrintGrants(gWatchOut);
    }

    for (auto iter = _multi_sw_alloc_vcs.begin(); iter != _multi_sw_alloc_vcs.end(); ++iter) {

        int const time = iter->first;
        if (time >= 0) {
            break;
        }
        iter->first = GetSimTime() + _sw_alloc_delay - 1;

        int const input = iter->second.first.first;
        assert((input >= 0) && (input < _inputs));
        int const vc = iter->second.first.second;
        assert((vc >= 0) && (vc < _vcs));

        if (!iter->second.second.empty()) {
            assert(iter->second.second.front() < -1);
            continue;
        }

        MCBuffer *const cur_buf = _mc_buf[input];
        assert(!cur_buf->Empty(vc));
        assert(cur_buf->GetState(vc) == VC::active);

        FocusFlit *const f = static_cast<FocusFlit*>(cur_buf->FrontFlit(vc));
        assert(f);
        assert(f->vc == vc);
        assert(f->GetFanoutFlitNum(GetID()) >= 1);

        // int const expanded_input = input * _input_speedup + vc % _input_speedup;
        int const expanded_input = input * _vcs + vc;
        vector<int> expanded_outputs = _multi_sw_allocator->MultiOutputAssigned(expanded_input);

        // Check whether our input port grant the output channel
        bool input_grant_succeed = true;
        for (int it: expanded_outputs) {
            input_grant_succeed &= it >= 0;
        }

        if (input_grant_succeed) {

            bool vc_grant_succeed = true;
            // Checkout whether our virtual channel grant the output channel
            for (int expanded_output: expanded_outputs) {
                // assert((expanded_output % _output_speedup) == (input % _output_speedup));
                int const granted_vc = _multi_sw_allocator->ReadRequest(expanded_input, expanded_output);
                vc_grant_succeed &= granted_vc == vc;
            }

            if (vc_grant_succeed) {
                assert(expanded_outputs.size() == f->GetFanoutFlitNum(GetID()));
                for (int expanded_output: expanded_outputs) {
                    if (f->watch)
                    {
                        *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                    << "Assigning output " << (expanded_output / _output_speedup)
                                    << "." << (expanded_output % _output_speedup)
                                    << " to VC " << vc
                                    << " at input " << input
                                    << "." << (vc % _input_speedup)
                                    << ". [Multicast Flit]" << endl;
                    }
                    // FIXME: What's this ??
                    // _sw_rr_offset[expanded_input] = (vc + _input_speedup) % _vcs;
                    // assert(false);
                    iter->second.second.push_back(expanded_output);
                }
            } else {
                if (f->watch) {
                    *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                << "Switch allocation failed for VC " << vc
                                << " at input " << input
                                << ": Granted to VCs" << ". [Multicast Flit]" << endl;
                }
                iter->second.second.push_back(STALL_CROSSBAR_CONFLICT);
            }
        } else {
            if (f->watch) {
                *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                           << "Switch allocation failed for VC " << vc
                           << " at input " << input
                           << ": No output granted. [Multicast Flit]" << endl;
            }
            iter->second.second.push_back(STALL_CROSSBAR_CONFLICT);
        }
    }

    assert(!_speculative && (_sw_alloc_delay <= 1));
}


void MCRouter::_MultiSWAllocUpdate() {

    while (!_multi_sw_alloc_vcs.empty()) {
        auto& item = _multi_sw_alloc_vcs.front();

        int const time = item.first;
        if ((time < 0) || (GetSimTime() < time)) {
            break;
        }
        assert(GetSimTime() == time);

        int const input = item.second.first.first;
        assert((input >= 0) && (input < _inputs));
        int const vc = item.second.first.second;
        assert((vc >= 0) && (vc < _vcs));

        MCBuffer *const cur_buf = _mc_buf[input];
        assert(!cur_buf->Empty(vc));
        assert(cur_buf->GetState(vc) == VC::active);

        FocusFlit *const f = static_cast<FocusFlit*>(cur_buf->FrontFlit(vc));
        assert(f);
        assert(f->vc == vc);
        assert(f->GetFanoutFlitNum(GetID()) >= 1);

        if (f->watch)
        {
            *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                        << "Completed switch allocation for VC " << vc
                        << " at input " << input
                        << " (front: " << f->id
                        << "). [multicast]" << endl;
        }

        vector<int> expanded_outputs = item.second.second;
        assert(!expanded_outputs.empty());

        bool grant_succeed = true;
        for (int it: expanded_outputs) {
            grant_succeed &= it >= 0;
        }

        if (grant_succeed) {
            int const expanded_input = input * _input_speedup + vc % _input_speedup;
            int osize = expanded_outputs.size();

            assert(osize == cur_buf->getMultiOutputs(vc).size());
            assert(osize == f->GetFanoutFlitNum(GetID()));

            vector<FocusFlit*> fanout_flits = f->GetFanoutFlits(GetID());
            map<int, int> out_pairs = cur_buf->getMultiOutputs(vc);                 // output port -> output vc

            map<int, int> port_to_target = cur_buf->getPortTargetMap(vc);           // output port -> target
            map<int, FocusFlit*> target_to_flit;                                   // target -> fanout flits
            for (FocusFlit* f: fanout_flits) {
                target_to_flit.insert(make_pair(f->SegEnd(), f));
            }

            assert(fanout_flits.size() == osize && port_to_target.size() == osize && out_pairs.size() == osize);

            // f is a dummy flit who won't reach the end, delete it
            if (!f->IsRealFlit() && f->IsRetired(GetID())) {
                // if (!(std::find(fanout_flits.begin(), fanout_flits.end(), f) == fanout_flits.end() && f->SegEnd() == GetID())) {
                //     std::cerr << f->id << std::endl;
                //     std::cerr << f->SegStart() << " " << f->SegEnd() << std::endl;
                //     std::cerr << "gid: " << GetID() << std::endl;
                //     std::cerr << f->pkt.fid << std::endl;
                // }
                // std::cerr << f->id << " " << f->SegEnd() << " " << GetID() << std::endl;
                // assert(f->SegEnd() == GetID());
                assert(std::find(fanout_flits.begin(), fanout_flits.end(), f) == fanout_flits.end());
                f->Free();
            }

            // Allocator doesn't guarantee the order of requests
            // out_pairs & expanded_outputs & duplicated_flits should be the same order

            for (int i = 0; i < osize; ++i) {
                int expanded_output = expanded_outputs[i];
                int const output = expanded_output / _output_speedup;

                assert((output >= 0) && (output < _outputs));
                assert(out_pairs.count(output) == 1);

                BufferState *const dest_buf = _next_buf[output];
                int match_vc = out_pairs[output];
                assert((match_vc >= 0) && (match_vc < _vcs));

                if (f->watch) {
                    *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                            << "  Scheduling switch connection from input " << input
                            << "." << (vc % _input_speedup)
                            << " to output " << output
                            << "." << (expanded_output % _output_speedup)
                            << "." << endl;
                }

                // assert(outport_to_flit.count(output) == 1);
                int target = port_to_target[output];
                FocusFlit* fanout_flit = target_to_flit[target];
                fanout_flit->hops = f->hops + 1;
                fanout_flit->vc = match_vc;

                dest_buf->SendingFlit(fanout_flit);
                _crossbar_flits.push_back(make_pair(-1, make_pair(fanout_flit, make_pair(expanded_input, expanded_output))));
            }

            if (_out_queue_credits.count(input) == 0) {
                _out_queue_credits.insert(make_pair(input, Credit::New()));
            }
            _out_queue_credits.find(input)->second->vc.insert(vc);

            // Remove the source flit
            cur_buf->RemoveFlit(vc);
            _bufferMonitor->read(input, f);

            if (cur_buf->Empty(vc)) {
                if (f->watch) {
                    *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                << "  Cancelling held connection from input " << input
                                << "." << (expanded_input % _input_speedup)
                                << " to its outputs "
                                << ": No more flits. [Multicast Flit]" << endl;
                }
                _switch_hold_vc[expanded_input] = -1;
                _switch_hold_in[expanded_input] = -1;
                for (int expanded_output: expanded_outputs) {
                    _switch_hold_out[expanded_output] = -1;
                }
                if (f->tail) {
                    cur_buf->SetState(vc, VC::idle);
                }
            } else {
                FocusFlit *const nf = static_cast<FocusFlit*>(cur_buf->FrontFlit(vc));
                assert(nf);
                assert(nf->vc == vc);
                assert(nf->GetFanoutFlitNum(GetID()) >= 1);
                if (f->tail) {
                    assert(nf->head);
                    if (f->watch) {
                        *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                                   << "  Cancelling held connection from input " << input
                                   << "." << (expanded_input % _input_speedup)
                                   << " to its outputs"
                                   << ": End of packet." << endl;
                    }
                    _switch_hold_vc[expanded_input] = -1;
                    _switch_hold_in[expanded_input] = -1;
                    for (int expanded_output: expanded_outputs) {
                        _switch_hold_out[expanded_output] = -1;
                    }
                    cur_buf->SetState(vc, VC::routing);
                    _multi_route_vcs.push_back(make_pair(-1, item.second.first));

                } else {
                    _multi_sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, vector<int>())));
                }
            }

        } else {
            if (f->watch)
            {
                *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                        << "  No output port allocated." << endl;
            }
            _multi_sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, vector<int>())));
        }
        _multi_sw_alloc_vcs.pop_front();
    }

}


MCRouter::MCRouter(
    Configuration const & config, Module *parent,
    string const & name, int id, int inputs, int outputs
) : IQRouter(config, parent, name, id, inputs, outputs)
{

    string multi_vc_alloc_type = config.GetStr("multi_vc_allocator");
    Allocator* temp;
    temp = Allocator::NewAllocator(this, "multi_vc_allocator",
                                                  multi_vc_alloc_type,
                                                  _vcs * _inputs,
                                                  _vcs * _outputs);
    assert(typeid(*temp) == typeid(MultiOutputSparseAllocator));
    _multi_vc_allocator = static_cast<MultiOutputSparseAllocator*>(temp);

    if (!_multi_vc_allocator) {
      Error("Unknown vc_allocator type: " + multi_vc_alloc_type);
    }

    string multi_sw_alloc_type = config.GetStr("multi_sw_allocator");
    // FIXME: this is also changed with different encoding
    temp = Allocator::NewAllocator(this, "multi_sw_allocator",
                                                  multi_sw_alloc_type,
                                                  _inputs * _vcs,
                                                //   _inputs * _input_speedup,
                                                  _outputs * _output_speedup);
    assert(typeid(*temp) == typeid(MultiOutputSparseAllocator));
    _multi_sw_allocator = static_cast<MultiOutputSparseAllocator*>(temp);

    if (!_multi_sw_allocator) {
        Error("Unknown sw_allocator type: " + multi_sw_alloc_type);
    }

    _mc_buf.resize(_inputs);
    for (int i = 0; i < _inputs; ++i) {
        ostringstream module_name;
        module_name << "mc_buf_" << i;
        _mc_buf[i] = new MCBuffer(config, _outputs, this, module_name.str());
        module_name.str("");
    }

}

double MCRouter::ConflictFactor() const {
    // Calculate conflict factor based on switch allocation conflicts
    // This is a simplified implementation - in a real system, you might want to track
    // actual conflicts over time and return a more sophisticated metric
    
    int total_requests = 0;
    int total_conflicts = 0;
    
    // Count conflicts from switch allocation
    if (_multi_sw_allocator) {
        // This is a simplified approach - in practice you'd want to track actual conflicts
        // For now, return a reasonable default value
        return 0.1; // 10% conflict rate as a default
    }
    
    return 0.0; // No conflicts if no allocator
}