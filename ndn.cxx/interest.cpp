/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2013 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *	   Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "interest.h"
#include <boost/lexical_cast.hpp>

using namespace std;

namespace ndn {

Interest::Interest ()
  // m_name
  : m_maxSuffixComponents (Interest::ncomps)
  , m_minSuffixComponents (Interest::ncomps)
  , m_answerOriginKind (AOK_DEFAULT)
  , m_interestLifetime (boost::posix_time::seconds (-1.0))
  , m_scope (NO_SCOPE)
  , m_childSelector (CHILD_DEFAULT)
  // m_publisherKeyDigest
{
}

Interest::Interest (const Name &name)
  : m_name (name)
  , m_maxSuffixComponents (Interest::ncomps)
  , m_minSuffixComponents (Interest::ncomps)
  , m_answerOriginKind(AOK_DEFAULT)
  , m_interestLifetime (boost::posix_time::seconds (-1.0))
  , m_scope (NO_SCOPE)
  , m_childSelector (CHILD_DEFAULT)
  // m_publisherKeyDigest
{
}

Interest::Interest (const Interest &other)
{
  m_name = other.m_name;
  m_maxSuffixComponents = other.m_maxSuffixComponents;
  m_minSuffixComponents = other.m_minSuffixComponents;
  m_answerOriginKind = other.m_answerOriginKind;
  m_interestLifetime = other.m_interestLifetime;
  m_scope = other.m_scope;
  m_childSelector = other.m_childSelector;
  m_publisherPublicKeyDigest = other.m_publisherPublicKeyDigest;
}

Interest::Interest (const ccn_parsed_interest *pi)
  : m_maxSuffixComponents (Interest::ncomps)
  , m_minSuffixComponents (Interest::ncomps)
  , m_answerOriginKind (AOK_DEFAULT)
  , m_interestLifetime (boost::posix_time::seconds (-1.0))
  , m_scope (NO_SCOPE)
  , m_childSelector (CHILD_DEFAULT)
{
  if (pi != NULL)
  {
    m_maxSuffixComponents = pi->max_suffix_comps;
    m_minSuffixComponents = pi->min_suffix_comps;
    switch(pi->orderpref)
      {
      case 0: m_childSelector = CHILD_LEFT; break;
      case 1: m_childSelector = CHILD_RIGHT; break;
      default: m_childSelector = CHILD_DEFAULT; break;
      }

    switch(pi->answerfrom)
    {
      case 0x1: m_answerOriginKind = AOK_CS; break;
      case 0x2: m_answerOriginKind = AOK_NEW; break;
      case 0x3: m_answerOriginKind = AOK_DEFAULT; break;
      case 0x4: m_answerOriginKind = AOK_STALE; break;
      case 0x10: m_answerOriginKind = AOK_EXPIRE; break;
      default: break;
    }
    m_scope = static_cast<Scope> (pi->scope);
  }

  /// @todo copy publisher key digest
}

bool
Interest::operator == (const Interest &other)
{
  return
       m_name == other.m_name
    && m_maxSuffixComponents == other.m_maxSuffixComponents
    && m_minSuffixComponents == other.m_minSuffixComponents
    && m_answerOriginKind == other.m_answerOriginKind
    && m_interestLifetime == other.m_interestLifetime
    && m_scope == other.m_scope
    && m_childSelector == other.m_childSelector;
}

} // ndn
