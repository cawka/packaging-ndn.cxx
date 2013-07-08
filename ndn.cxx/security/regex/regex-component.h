/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */


#ifndef NDN_REGEX_COMPONENT_H
#define NDN_REGEX_COMPONENT_H

#include <string>
#include <boost/regex.hpp>

#include "ndn.cxx/fields/name.h"

#include "regex-parser.h"


using namespace std;


namespace ndn
{

namespace regex
{
    
  class RegexComponent : public RegexMatcher
  {
  public:
    ///////////////////////////////////////////////////////////////////////////////
    //                              CONSTRUCTORS                                 //
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Create a RegexComponent matcher from expr
     * @param expr The standard regular expression to match a component
     * @param exact The flag to provide exact match
     * @param backRefNum The starting back reference number
     */
    RegexComponent(const string expr, int backRefNum, bool exact = true)
      : RegexMatcher (expr, EXPR_COMPONENT, backRefNum),
        m_exact(exact)
    {};
    
    virtual ~RegexComponent() {};
    
    /**
     * @brief Compile the regular expression to generate the more matchers when necessary
     * @returns true if compiling succeeds
     */
    virtual bool Compile() {};

    /**
     * @brief check if the pattern match the part of name
     * @param name name against which the pattern is matched
     * @param offset starting index of matching
     * @param len number of components to be matched
     * @returns true if match succeeds
     */
    virtual bool Match(Name name, const int & offset, const int & len = 1);
    
  private:
    bool m_exact;
  };

}//regex
    
}//ndn

#endif