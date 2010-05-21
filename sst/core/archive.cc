// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization/archive.h"
#include "sst/core/serialization/types.h"

#include <iostream>
#include <string>
#include <fstream>

#include "sst/core/archive.h"
#include "sst/core/simulation.h"
#include <sst/core/sync.h>
#include <sst/core/factory.h>
#include <sst/core/stopEvent.h>
#include <sst/core/exit.h>
#include <sst/core/compEvent.h>
#include <sst/core/config.h>
#include <sst/core/graph.h>
#include <sst/core/timeLord.h>

using namespace SST;

Archive::Archive(std::string ttype, std::string filename) :
    filename(filename)
{
    if (ttype.compare("xml") &&
        ttype.compare("text") &&
        ttype.compare("bin")) {
        fprintf(stderr, "Serialization type %s unknown.  Defaulting to bin\n",
                ttype.c_str());
        type = "bin";
    } else {
        type = ttype;
    }
}


Archive::~Archive()
{
}


void
Archive::SaveSimulation(Simulation* simulation)
{
    std::string savename = filename + "." + type;
    std::ofstream ofs(savename.c_str());

    if (type == "xml") {
        boost::archive::polymorphic_xml_oarchive oa(ofs);
        oa << BOOST_SERIALIZATION_NVP(simulation);
    } else if (type == "text") {
        boost::archive::polymorphic_text_oarchive oa(ofs);
        oa << BOOST_SERIALIZATION_NVP(simulation);
    } else if (type == "bin") {
        boost::archive::polymorphic_binary_oarchive oa(ofs);
        oa << BOOST_SERIALIZATION_NVP(simulation);
    } else {
        abort();
    }
}


Simulation* 
Archive::LoadSimulation(void)
{
    std::string loadname = filename + "." + type;
    std::ifstream ifs(loadname.c_str());
    Simulation* simulation;

    if (type == "xml") {
        boost::archive::polymorphic_xml_iarchive ia(ifs);
        ia >> BOOST_SERIALIZATION_NVP(simulation);
    } else if (type == "text") {
        boost::archive::polymorphic_text_iarchive ia(ifs);
        ia >> BOOST_SERIALIZATION_NVP(simulation);
    } else if (type == "bin") {
        boost::archive::polymorphic_binary_iarchive ia(ifs);
        ia >> BOOST_SERIALIZATION_NVP(simulation);
    } else {
        abort();
    }

    return simulation;
}
