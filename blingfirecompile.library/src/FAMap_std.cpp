/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "blingfire-compile_src_pch.h"
#include "FAConfig.h"
#include "FAMap_std.h"

namespace BlingFire
{


FAMap_std::FAMap_std ()
{}

FAMap_std::~FAMap_std ()
{}

const int * FAMap_std::Get (const int Key) const
{
    std::map < int, int >::const_iterator I = m_map.find (Key);

    if (I != m_map.end ()) {

        /// TODO: change, this line is bad
        return & I->second;
    }

    return NULL;
}

void FAMap_std::Set (const int Key, const int Value)
{
    std::pair < std::map <int, int>::iterator, bool > status = 
        m_map.insert (std::make_pair (Key, Value));

    if (false == status.second) {
        (status.first)->second = Value;
    }
}

const int * FAMap_std::Next (int * pKey) const
{
    DebugLogAssert (pKey);

    const int NextKey = 1 + *pKey;

    std::map < int, int >::const_iterator I = m_map.lower_bound (NextKey);

    if (I != m_map.end ()) {

        /// TODO: change, this line is bad
        *pKey = I->first;
        return & I->second;
    }

    return NULL;
}

const int * FAMap_std::Prev (int * pKey) const
{
    DebugLogAssert (pKey);

    const int NextKey = 1 + *pKey;

    std::map < int, int >::const_iterator I = m_map.lower_bound (NextKey);

    if (I != m_map.end ()) {

        /// TODO: change, this line is bad
        *pKey = I->first;
        return & I->second;
    }

    return NULL;
}

void FAMap_std::Remove (const int Key)
{
    m_map.erase (Key);
}

void FAMap_std::Clear ()
{
    m_map.clear ();
}

}

