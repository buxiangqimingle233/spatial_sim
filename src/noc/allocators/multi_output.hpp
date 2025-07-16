
#ifndef __MULTI_OUTPUT_HPP__
#define __MULTI_OUTPUT_HPP__

#include <vector>

#include "allocator.hpp"
#include "wavefront.hpp"

//==================================================
// A multi-output sparse allocator allocate as much
// as possible for the selected input_and_vc
//==================================================


class MultiOutputSparseAllocator : public SparseAllocator {

protected:
  vector<map<int, int>> _multi_inmatch;
  vector<map<int, int>> _multi_outmatch;

public:
  vector<int> MultiOutputAssigned( int in ) {
    assert(in >= 0 && in < _inputs);
    assert(_in_occ.count(in) > 0 && _multi_inmatch[in].size() > 0);
    // return _multi_inmatch[in];
    vector<int> ret;
    for (auto it: _multi_inmatch[in]) {
      ret.push_back(it.second);
    }
    return ret;
  }

  virtual void Allocate( ) override {
    // Initialize the outputs
    for (int in: _in_occ) {
      for (auto it = _in_req[in].begin(); it != _in_req[in].end(); ++it) {
        int out = it->first;
        _multi_inmatch[in][out] = -1;
        _multi_outmatch[out][in] = -1;
      }
    }

    // Allocate outputs based on the order of _in_occ set 
    for (int in: _in_occ) {
      bool all_grant_succeed = true;
      for (auto it = _in_req[in].begin(); it != _in_req[in].end(); ++it) {
        int out = it->first;
        for (auto check_iter = _multi_outmatch[out].begin(); check_iter != _multi_outmatch[out].end(); ++check_iter) {
          all_grant_succeed &= check_iter->second == -1;
        }
      }
      if (all_grant_succeed) {
        for (auto it = _in_req[in].begin(); it != _in_req[in].end(); ++it) {
          int out = it->first;
          _multi_inmatch[in][out] = out;
          _multi_outmatch[out][in] = out;
        }
      }
    }
  }

  virtual void Clear( ) override {
    SparseAllocator::Clear();
    for (auto it = _multi_inmatch.begin(); it != _multi_inmatch.end(); ++it) {
      it->clear();
    }
    for (auto it = _multi_outmatch.begin(); it != _multi_outmatch.end(); ++it) {
      it->clear();
    }
  }


  void PrintRequests(ostream *os) const override
  {
    map<int, sRequest>::const_iterator iter;

    if (!os)
      os = &cout;

    *os << "Input requests = [ ";
    for (int input = 0; input < _inputs; ++input)
    {
      if (!_in_req[input].empty())
      {
        *os << input << " -> [ ";
        for (iter = _in_req[input].begin();
            iter != _in_req[input].end(); iter++)
        {
          *os << iter->second.port << "@" << iter->second.label << "@" << iter->second.in_pri << " ";
        }
        *os << "]  ";
      }
    }
    *os << "], output requests = [ ";
    for (int output = 0; output < _outputs; ++output)
    {
      if (!_out_req[output].empty())
      {
        *os << output << " -> ";
        *os << "[ ";
        for (iter = _out_req[output].begin();
            iter != _out_req[output].end(); iter++)
        {
          *os << iter->second.port << "@" << iter->second.label << "@" << iter->second.out_pri << " ";
        }
        *os << "]  ";
      }
    }
    *os << "]." << endl;
  }

  void PrintGrants( ostream * os = NULL ) const {
    if (!os)
      os = &cout;

    *os << "Input grants = [ ";
    for (int input = 0; input < _inputs; ++input)
    {
      if (_multi_inmatch[input].size() > 0)
      {
        *os << input << " -> [ ";
        for (auto match: _multi_inmatch[input]) {
          *os << match.first << "-" << match.second << " ";
        }
        *os << "] ";
      }
    }
    *os << "], output grants = [ ";
    for (int output = 0; output < _outputs; ++output)
    {
      if (_multi_outmatch[output].size() > 0)
      {
        *os << output << " -> [ ";
        for (auto match: _multi_outmatch[output]) {
          *os << match.first << "-" << match.second << " ";
        }
        *os << "] ";
      }
    }
    *os << "]." << endl;
  }

  MultiOutputSparseAllocator( Module *parent, const string& name, int inputs, int outputs )
    : SparseAllocator(parent, name, inputs, outputs)
  { 
    _multi_inmatch.resize(inputs);
    _multi_outmatch.resize(outputs);    
  };

};


#endif