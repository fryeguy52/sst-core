// -*- c++ -*-

// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/warnmacros.h"

DISABLE_WARN_DEPRECATED_REGISTER
#include <Python.h>
REENABLE_WARNING

#include <string.h>

#include "sst/core/model/python3/pymodel.h"
#include "sst/core/model/python3/pymodel_comp.h"
#include "sst/core/model/python3/pymodel_link.h"

#include "sst/core/sst_types.h"
#include "sst/core/simulation.h"
#include "sst/core/component.h"
#include "sst/core/subcomponent.h"
#include "sst/core/configGraph.h"

using namespace SST::Core;
extern SST::Core::SSTPythonModelDefinition *gModel;


extern "C" {


ConfigComponent* ComponentHolder::getSubComp(const std::string& name, int slot_num)
{
    for ( auto &sc : getComp()->subComponents ) {
        if ( sc.name == name && sc.slot_num == slot_num)
            return &sc;
    }
    return nullptr;
}

ComponentId_t ComponentHolder::getID()
{
    return getComp()->id;
}

const char* PyComponent::getName() const {
    return name;
}


ConfigComponent* PyComponent::getComp() {
    return &(gModel->getGraph()->getComponentMap()[id]);
}


PyComponent* PyComponent::getBaseObj() {
    return this;
}

int PyComponent::compare(ComponentHolder *other) {
    PyComponent *o = dynamic_cast<PyComponent*>(other);
    if ( o ) {
        return (id < o->id) ? -1 : (id > o->id) ? 1 : 0;
    }
    return 1;
}


const char* PySubComponent::getName() const {
    return name;
}

int PySubComponent::getSlot() const {
    return slot;
}

ConfigComponent* PySubComponent::getComp() {
    return parent->getSubComp(name,slot);
}


PyComponent* PySubComponent::getBaseObj() {
    return parent->getBaseObj();
}


int PySubComponent::compare(ComponentHolder *other) {
    PySubComponent *o = dynamic_cast<PySubComponent*>(other);
    if ( o ) {
        int pCmp = parent->compare(o->parent);
        if ( pCmp == 0 ) /* Parents are equal */
            pCmp = strcmp(name, o->name);
        return pCmp;
    }
    return -11;
}



static int compInit(ComponentPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *name, *type;
    ComponentId_t useID = UNSET_COMPONENT_ID;
    if ( !PyArg_ParseTuple(args, "ss|k", &name, &type, &useID) )
        return -1;

    PyComponent *obj = new PyComponent(self);
    self->obj = obj;
    if ( useID == UNSET_COMPONENT_ID ) {
        obj->name = gModel->addNamePrefix(name);
        obj->id = gModel->addComponent(obj->name, type);
        gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating component [%s] of type [%s]: id [%" PRIu64 "]\n", name, type, obj->id);
    } else {
        obj->name = name;
        obj->id = useID;
    }

    return 0;
}


static void compDealloc(ComponentPy_t *self)
{
    if ( self->obj ) delete self->obj;
    Py_TYPE(self)->tp_free((PyObject*)self);
}



static PyObject* compAddParam(PyObject *self, PyObject *args)
{
    char *param = nullptr;
    PyObject *value = nullptr;
    if ( !PyArg_ParseTuple(args, "sO", &param, &value) )
        return nullptr;

    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;

    PyObject *vstr = PyObject_Str(value);
    c->addParameter(param, PyUnicode_AsUTF8(vstr), true);
    Py_XDECREF(vstr);

    return PyLong_FromLong(0);
}


static PyObject* compAddParams(PyObject *self, PyObject *args)
{
    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;

    if ( !PyDict_Check(args) ) {
        return nullptr;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *val;
    long count = 0;

    while ( PyDict_Next(args, &pos, &key, &val) ) {
        PyObject *kstr = PyObject_Str(key);
        PyObject *vstr = PyObject_Str(val);
        c->addParameter(PyUnicode_AsUTF8(kstr), PyUnicode_AsUTF8(vstr), true);
        Py_XDECREF(kstr);
        Py_XDECREF(vstr);
        count++;
    }
    return PyLong_FromLong(count);
}


static PyObject* compSetRank(PyObject *self, PyObject *args)
{
    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;

    PyErr_Clear();

    unsigned long rank = (unsigned long)-1;
    unsigned long thread = (unsigned long)0;

    if ( !PyArg_ParseTuple(args, "k|k", &rank, &thread) ) {
        return nullptr;
    }

    c->setRank(RankInfo(rank, thread));

    return PyLong_FromLong(0);
}


static PyObject* compSetWeight(PyObject *self, PyObject *arg)
{
    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;

    PyErr_Clear();
    double weight = PyFloat_AsDouble(arg);
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        exit(-1);
    }

    c->setWeight(weight);

    return PyLong_FromLong(0);
}


static PyObject* compAddLink(PyObject *self, PyObject *args)
{
    ConfigComponent *c = getComp(self);
    ComponentId_t id = c->id;

    PyObject *plink = nullptr;
    char *port = nullptr, *lat = nullptr;


    if ( !PyArg_ParseTuple(args, "O!s|s", &PyModel_LinkType, &plink, &port, &lat) ) {
        return nullptr;
    }
    LinkPy_t* link = (LinkPy_t*)plink;
    if ( nullptr == lat ) lat = link->latency;
    if ( nullptr == lat ) return nullptr;


    gModel->getOutput()->verbose(CALL_INFO, 4, 0, "Connecting component %" PRIu64 " to Link %s (lat: %s)\n", id, link->name, lat);
    gModel->addLink(id, link->name, port, lat, link->no_cut);

    return PyLong_FromLong(0);
}


static PyObject* compGetFullName(PyObject *self, PyObject *UNUSED(args))
{
    return PyBytes_FromString(getComp(self)->name.c_str());
}

static PyObject* compCompare(PyObject *obj0, PyObject *obj1, int op) {
    PyObject *result;
    bool cmp = false;
    switch(op) {
        case Py_LT: cmp = ((ComponentPy_t*)obj0)->obj->compare(((ComponentPy_t*)obj1)->obj) == -1;
        case Py_LE: cmp = ((ComponentPy_t*)obj0)->obj->compare(((ComponentPy_t*)obj1)->obj) != 1;
        case Py_EQ: cmp = ((ComponentPy_t*)obj0)->obj->compare(((ComponentPy_t*)obj1)->obj) == 0;
        case Py_NE: cmp = ((ComponentPy_t*)obj0)->obj->compare(((ComponentPy_t*)obj1)->obj) != 0;
        case Py_GT: cmp = ((ComponentPy_t*)obj0)->obj->compare(((ComponentPy_t*)obj1)->obj) == 1;
        case Py_GE: cmp = ((ComponentPy_t*)obj0)->obj->compare(((ComponentPy_t*)obj1)->obj) != -1;
    }
    result = cmp ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}


static PyObject* compSetSubComponent(PyObject *self, PyObject *args)
{
    char *name = nullptr, *type = nullptr;
    int slot = 0;
    
    if ( !PyArg_ParseTuple(args, "ss|i", &name, &type, &slot) )
        return nullptr;

    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;

    PyComponent *baseComp = ((ComponentPy_t*)self)->obj->getBaseObj();
    ComponentId_t subC_id = SUBCOMPONENT_ID_CREATE(baseComp->id, ++(baseComp->subCompId));
    if ( nullptr != c->addSubComponent(subC_id, name, type, slot) ) {
        PyObject *argList = Py_BuildValue("Ossi", self, name, type, slot);
        PyObject *subObj = PyObject_CallObject((PyObject*)&PyModel_SubComponentType, argList);
        Py_DECREF(argList);
        return subObj;
    }

    char errMsg[1024] = {0};
    snprintf(errMsg, sizeof(errMsg)-1, "Failed to create subcomponent %s on %s.  Already attached a subcomponent at that slot name and number?\n", name, c->name.c_str());
    PyErr_SetString(PyExc_RuntimeError, errMsg);
    return nullptr;
}

static PyObject* compSetCoords(PyObject *self, PyObject *args)
{
    std::vector<double> coords(3, 0.0);
    if ( !PyArg_ParseTuple(args, "d|dd", &coords[0], &coords[1], &coords[2]) ) {
        PyObject* list = nullptr;
        if ( PyArg_ParseTuple(args, "O!", &PyList_Type, &list) && PyList_Size(list) > 0 ) {
            coords.clear();
            for ( Py_ssize_t i = 0 ; i < PyList_Size(list) ; i++ ) {
                coords.push_back(PyFloat_AsDouble(PyList_GetItem(list, 0)));
                if ( PyErr_Occurred() ) goto error;
            }
        } else if ( PyArg_ParseTuple(args, "O!", &PyTuple_Type, &list) && PyTuple_Size(list) > 0 ) {
            coords.clear();
            for ( Py_ssize_t i = 0 ; i < PyTuple_Size(list) ; i++ ) {
                coords.push_back(PyFloat_AsDouble(PyTuple_GetItem(list, 0)));
                if ( PyErr_Occurred() ) goto error;
            }
        } else {
error:
            PyErr_SetString(PyExc_TypeError, "compSetCoords() expects arguments of 1-3 doubles, or a list/tuple of doubles");
            return nullptr;
        }
    }

    ConfigComponent *c = getComp(self);
    if ( nullptr == c ) return nullptr;
    c->setCoordinates(coords);

    return PyLong_FromLong(0);
}

static PyObject* compEnableAllStatistics(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    PyObject*     statParamDict = nullptr;
    ConfigComponent *c = getComp(self);

    PyErr_Clear();

    // Parse the Python Args and get optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "|O!", &PyDict_Type, &statParamDict);

    if (argOK) {
        c->enableStatistic(STATALLFLAG);

        // Generate and Add the Statistic Parameters
        for ( auto p : generateStatisticParameters(statParamDict) ) {
            c->addStatisticParameter(STATALLFLAG, p.first, p.second);
        }

    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}


static PyObject* compEnableStatistics(PyObject *self, PyObject *args)
{
    int           argOK = 0;
    PyObject*     statList = nullptr;
    PyObject*     statParamDict = nullptr;
    Py_ssize_t    numStats = 0;
    ConfigComponent *c = getComp(self);

    PyErr_Clear();

    // Parse the Python Args and get A List Object and the optional Stat Params (as a Dictionary)
    argOK = PyArg_ParseTuple(args, "O!|O!", &PyList_Type, &statList, &PyDict_Type, &statParamDict);

    if (argOK) {
        // Generate the Statistic Parameters
        auto params = generateStatisticParameters(statParamDict);

        // Make sure we have a list
        if ( !PyList_Check(statList) ) {
            return nullptr;
        }

        // Get the Number of Stats in the list, and enable them separately,
        // also set their parameters
        numStats = PyList_Size(statList);
        for (uint32_t x = 0; x < numStats; x++) {
            PyObject* pylistitem = PyList_GetItem(statList, x);
            PyObject* pyname = PyObject_Str(pylistitem);

            c->enableStatistic(PyUnicode_AsUTF8(pyname));

            // Add the parameters
            for ( auto p : params ) {
                c->addStatisticParameter(PyBytes_AsString(pyname), p.first, p.second);
            }

            Py_XDECREF(pyname);
        }
    } else {
        // ParseTuple Failed, return NULL for error
        return nullptr;
    }
    return PyLong_FromLong(0);
}


static PyMethodDef componentMethods[] = {
    {   "addParam",
        compAddParam, METH_VARARGS,
        "Adds a parameter(name, value)"},
    {   "addParams",
        compAddParams, METH_O,
        "Adds Multiple Parameters from a dict"},
    {   "setRank",
        compSetRank, METH_VARARGS,
        "Sets which rank on which this component should sit"},
    {   "setWeight",
        compSetWeight, METH_O,
        "Sets the weight of the component"},
    {   "addLink",
        compAddLink, METH_VARARGS,
        "Connects this component to a Link"},
    {   "getFullName",
        compGetFullName, METH_NOARGS,
        "Returns the full name, after any prefix, of the component."},
    {   "enableAllStatistics",
        compEnableAllStatistics, METH_VARARGS,
        "Enable all Statistics in the component with optional parameters"},
    {   "enableStatistics",
        compEnableStatistics, METH_VARARGS,
        "Enables Multiple Statistics in the component with optional parameters"},
    {   "setSubComponent",
        compSetSubComponent, METH_VARARGS,
        "Bind a subcomponent to slot <name>, with type <type>"},
    {   "setCoordinates",
        compSetCoords, METH_VARARGS,
        "Set (X,Y,Z) coordinates of this component, for use with visualization"},
    {   nullptr, nullptr, 0, nullptr }
};


PyTypeObject PyModel_ComponentType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "sst.Component",           /* tp_name */
    sizeof(ComponentPy_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)compDealloc,   /* tp_dealloc */
    nullptr,                   /* tp_vectorcall_offset */
    nullptr,                   /* tp_getattr */
    nullptr,                   /* tp_setattr */
    nullptr,                   /* tp_as_async */
    nullptr,                   /* tp_repr */
    nullptr,                   /* tp_as_number */
    nullptr,                   /* tp_as_sequence */
    nullptr,                   /* tp_as_mapping */
    nullptr,                   /* tp_hash */
    nullptr,                   /* tp_call */
    nullptr,                   /* tp_str */
    nullptr,                   /* tp_getattro */
    nullptr,                   /* tp_setattro */
    nullptr,                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "SST Component",           /* tp_doc */
    nullptr,                   /* tp_traverse */
    nullptr,                   /* tp_clear */
    compCompare,               /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    nullptr,                   /* tp_iter */
    nullptr,                   /* tp_iternext */
    componentMethods,          /* tp_methods */
    nullptr,                   /* tp_members */
    nullptr,                   /* tp_getset */
    nullptr,                   /* tp_base */
    nullptr,                   /* tp_dict */
    nullptr,                   /* tp_descr_get */
    nullptr,                   /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)compInit,        /* tp_init */
    nullptr,                   /* tp_alloc */
    nullptr,                   /* tp_new */
    nullptr,                   /* tp_free */
    nullptr,                   /* tp_is_gc */
    nullptr,                   /* tp_bases */
    nullptr,                   /* tp_mro */
    nullptr,                   /* tp_cache */
    nullptr,                   /* tp_subclasses */
    nullptr,                   /* tp_weaklist */
    nullptr,                   /* tp_del */
    0,                         /* tp_version_tag */
    nullptr,                   /* tp_finalize */
};





static int subCompInit(ComponentPy_t *self, PyObject *args, PyObject *UNUSED(kwds))
{
    char *name, *type;
    int slot;
    PyObject *parent;
    if ( !PyArg_ParseTuple(args, "Ossi", &parent, &name, &type, &slot) )
        return -1;

    PySubComponent *obj = new PySubComponent(self);
    obj->parent = ((ComponentPy_t*)parent)->obj;

    obj->name = strdup(name);

    obj->slot = slot;
    
    self->obj = obj;
    Py_INCREF(obj->parent->pobj);

    gModel->getOutput()->verbose(CALL_INFO, 3, 0, "Creating subcomponent [%s] of type [%s]]\n", name, type);

    return 0;
}


static void subCompDealloc(ComponentPy_t *self)
{
    if ( self->obj ) {
        PySubComponent *obj = (PySubComponent*)self->obj;
        Py_XDECREF(obj->parent->pobj);
        delete self->obj;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}



static PyMethodDef subComponentMethods[] = {
    {   "addParam",
        compAddParam, METH_VARARGS,
        "Adds a parameter(name, value)"},
    {   "addParams",
        compAddParams, METH_O,
        "Adds Multiple Parameters from a dict"},
    {   "addLink",
        compAddLink, METH_VARARGS,
        "Connects this subComponent to a Link"},
    {   "enableAllStatistics",
        compEnableAllStatistics, METH_VARARGS,
        "Enable all Statistics in the component with optional parameters"},
    {   "enableStatistics",
        compEnableStatistics, METH_VARARGS,
        "Enables Multiple Statistics in the component with optional parameters"},
    {   "setSubComponent",
        compSetSubComponent, METH_VARARGS,
        "Bind a subcomponent to slot <name>, with type <type>"},
    {   "setCoordinates",
        compSetCoords, METH_VARARGS,
        "Set (X,Y,Z) coordinates of this component, for use with visualization"},
    {   nullptr, nullptr, 0, nullptr }
};


PyTypeObject PyModel_SubComponentType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "sst.SubComponent",        /* tp_name */
    sizeof(ComponentPy_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)subCompDealloc,/* tp_dealloc */
    nullptr,                   /* tp_vectorcall_offset */
    nullptr,                   /* tp_getattr */
    nullptr,                   /* tp_setattr */
    nullptr,                   /* tp_as_async */
    nullptr,                   /* tp_repr */
    nullptr,                   /* tp_as_number */
    nullptr,                   /* tp_as_sequence */
    nullptr,                   /* tp_as_mapping */
    nullptr,                   /* tp_hash */
    nullptr,                   /* tp_call */
    nullptr,                   /* tp_str */
    nullptr,                   /* tp_getattro */
    nullptr,                   /* tp_setattro */
    nullptr,                   /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "SST SubComponent",        /* tp_doc */
    nullptr,                   /* tp_traverse */
    nullptr,                   /* tp_clear */
    compCompare,               /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    nullptr,                   /* tp_iter */
    nullptr,                   /* tp_iternext */
    subComponentMethods,       /* tp_methods */
    nullptr,                   /* tp_members */
    nullptr,                   /* tp_getset */
    nullptr,                   /* tp_base */
    nullptr,                   /* tp_dict */
    nullptr,                   /* tp_descr_get */
    nullptr,                   /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)subCompInit,     /* tp_init */
    nullptr,                   /* tp_alloc */
    nullptr,                   /* tp_new */
    nullptr,                   /* tp_free */
    nullptr,                   /* tp_is_gc */
    nullptr,                   /* tp_bases */
    nullptr,                   /* tp_mro */
    nullptr,                   /* tp_cache */
    nullptr,                   /* tp_subclasses */
    nullptr,                   /* tp_weaklist */
    nullptr,                   /* tp_del */
    0,                         /* tp_version_tag */
    nullptr,                   /* tp_finalize */
};


}  /* extern C */


