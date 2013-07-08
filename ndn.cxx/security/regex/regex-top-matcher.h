/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef NDN_REGEX_TOP_MATCHER_H
#define NDN_REGEX_TOP_MATCHER_H

#include <string>

using namespace std;

namespace ndn
{

namespace regex
{
  class RegexTopMatcher: public RegexMatcher
  {
  public:
    RegexTopMatcher(const string expr, RegexBRManager *const backRefManager)
      : RegexMatcher(expr, EXPR_TOP, backRefManager)
    {};
    
    virtual ~RegexTopMatcher();

    virtual bool Compile();
    
    virtual bool Match(Name name, const int & offset, const int & len);

  private:
    
  };
}

}

#endif
