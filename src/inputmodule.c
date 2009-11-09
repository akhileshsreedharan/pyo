#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"

typedef struct {
    pyo_audio_HEAD
    int chnl;
    int modebuffer[2];
} Input;

static void Input_postprocessing_ii(Input *self) { POST_PROCESSING_II };
static void Input_postprocessing_ai(Input *self) { POST_PROCESSING_AI };
static void Input_postprocessing_ia(Input *self) { POST_PROCESSING_IA };
static void Input_postprocessing_aa(Input *self) { POST_PROCESSING_AA };

static void
_setProcMode(Input *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Input_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Input_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = Input_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Input_postprocessing_aa;
            break;
    }    
}

static void
_compute_next_data_frame(Input *self)
{   
    int i;
    float *tmp;
    tmp = Server_getInputBuffer((Server *)self->server);
    for (i=0; i<self->bufsize*self->nchnls; i++) {
        if ((i % self->nchnls) == self->chnl)
            self->data[(int)(i/self->nchnls)] = tmp[i];
    }    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Input_traverse(Input *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
Input_clear(Input *self)
{
    pyo_CLEAR
    return 0;
}

static void
Input_dealloc(Input* self)
{
    free(self->data);
    Input_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Input_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Input *self;
    self = (Input *)type->tp_alloc(type, 0);

    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON

    return (PyObject *)self;
}

static int
Input_init(Input *self, PyObject *args, PyObject *kwds)
{
    PyObject *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iOO", kwlist, &self->chnl, &multmp, &addtmp))
        return -1; 
 
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
            
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    _setProcMode(self);

    _compute_next_data_frame((Input *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Input_getServer(Input* self) { GET_SERVER };
static PyObject * Input_getStream(Input* self) { GET_STREAM };
static PyObject * Input_setMul(Input *self, PyObject *arg) { SET_MUL };	
static PyObject * Input_setAdd(Input *self, PyObject *arg) { SET_ADD };	

static PyObject * Input_play(Input *self) { PLAY };
static PyObject * Input_out(Input *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Input_stop(Input *self) { STOP };


static PyObject *
Input_multiply(Input *self, PyObject *arg)
{
    PyObject_CallMethod((PyObject *)self, "setMul", "O", arg);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
Input_inplace_multiply(Input *self, PyObject *arg)
{
    PyObject_CallMethod((PyObject *)self, "setMul", "O", arg);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
Input_add(Input *self, PyObject *arg)
{
    PyObject_CallMethod((PyObject *)self, "setAdd", "O", arg);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
Input_inplace_add(Input *self, PyObject *arg)
{
    PyObject_CallMethod((PyObject *)self, "setAdd", "O", arg);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyMemberDef Input_members[] = {
    {"server", T_OBJECT_EX, offsetof(Input, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Input, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Input, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Input, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Input_methods[] = {
    //{"getInput", (PyCFunction)Input_getTable, METH_NOARGS, "Returns input sound object."},
    {"getServer", (PyCFunction)Input_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Input_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Input_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Input_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Input_stop, METH_NOARGS, "Stops computing."},
	{"setMul", (PyCFunction)Input_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Input_setAdd, METH_O, "Sets oscillator add factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Input_as_number = {
    (binaryfunc)Input_add,                      /*nb_add*/
    0,                 /*nb_subtract*/
    (binaryfunc)Input_multiply,                 /*nb_multiply*/
    0,                   /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    0,                                          /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    0,                       /*nb_oct*/
    0,                       /*nb_hex*/
    (binaryfunc)Input_inplace_add,              /*inplace_add*/
    0,         /*inplace_subtract*/
    (binaryfunc)Input_inplace_multiply,         /*inplace_multiply*/
    0,           /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    0,              /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    0,      /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject InputType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyo.Input",         /*tp_name*/
    sizeof(Input),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Input_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Input_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Input objects. Retreive audio from an input channel.",           /* tp_doc */
    (traverseproc)Input_traverse,   /* tp_traverse */
    (inquiry)Input_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Input_methods,             /* tp_methods */
    Input_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Input_init,      /* tp_init */
    0,                         /* tp_alloc */
    Input_new,                 /* tp_new */
};
