#ifndef __MC_ROUTER_HPP_
#define __MC_ROUTER_HPP_

#include <deque>
#include <vector>
#include <map>
#include "iq_router.hpp"
#include "router.hpp"
#include "buffer.hpp"
#include "multi_output.hpp"

using namespace std;

class MCRouter : public IQRouter {

protected: 

    // Sperated Data Pane With Multicast [ one vc granting multiple outports ]
    vector<MCBuffer* > _mc_buf;

    // Seperated Control Pane For Multicast
    deque<pair<int, pair<int, int> > > _multi_route_vcs;
    deque<pair<int, pair<pair<int, int>, vector<int> > > > _multi_vc_alloc_vcs;
    // deque<pair<int, pair<pair<int, int>, vector<int> > > > _multi_sw_hold_vcs;
    deque<pair<int, pair<pair<int, int>, vector<int> > > > _multi_sw_alloc_vcs;

    // Sparse allocator
    MultiOutputSparseAllocator* _multi_sw_allocator;
    MultiOutputSparseAllocator* _multi_vc_allocator;

public:

    virtual void _InternalStep() override;

    void _FlitDispatch();    // This function replaces InputQueueing to dispatch incoming flits to different control panes. 
                             // Flits with more than one fanouts are processed by new functions headed with _multi.
                             // And other flits with only one fanout are precessed by original functions in IQRouter.

    void _MultiRouteEvaluate( );
    void _MultiVCAllocEvaluate( );
    void _MultiSWHoldEvaluate( );
    void _MultiSWAllocEvaluate( );
    void _MultiSwitchEvaluate( );

    void _MultiRouteUpdate( );
    void _MultiVCAllocUpdate( );
    void _MultiSWHoldUpdate( );
    void _MultiSWAllocUpdate( );
    void _MultiSwitchUpdate( );

    // Get conflict factor for router statistics
    double ConflictFactor() const;

    MCRouter( Configuration const & config, Module *parent, string const & name, int id,
              int inputs, int outputs );

};


#endif 