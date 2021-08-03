#include <stdexcept>

#ifdef _DEBUG
#undef _DEBUG
#include "PyUtils.h"
#include "PyDoc.h"
#define _DEBUG
#else 
#include "PyUtils.h"
#include "PyDoc.h"
#endif

#include <kiwi/Kiwi.h>

using namespace std;
using namespace kiwi;

static PyObject* gModule;

struct KiwiObject : py::CObject<KiwiObject>
{
	KiwiBuilder builder;
	Kiwi kiwi;

	static int init(KiwiObject *self, PyObject *args, PyObject *kwargs)
	{
		return py::handleExc([&]()
		{
			const char* modelPath = nullptr;
			size_t numThreads = 0, options = 3;
			int integrateAllomorph = -1, loadDefaultDict = -1;
			static const char* kwlist[] = { "num_workers", "model_path", "options", "integrate_allomorph", "load_default_dict", nullptr };
			if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|nznpp", (char**)kwlist,
				&numThreads, &modelPath, &options, &integrateAllomorph, &loadDefaultDict
			)) return -1;

			BuildOption boptions = (BuildOption)options;

			if (kwargs && PyDict_GetItemString(kwargs, "options"))
			{
				if (PyErr_WarnEx(PyExc_DeprecationWarning, "Argument `options` will be removed in future version. Use `integrate_allomorph` or `load_default_dict` instead.", 1)) return -1;
			}

			if (integrateAllomorph >= 0)
			{
				boptions = (boptions & ~BuildOption::integrateAllomorph) 
					| (integrateAllomorph ? BuildOption::integrateAllomorph : BuildOption::none);
			}

			if (loadDefaultDict >= 0)
			{
				boptions = (boptions & ~BuildOption::loadDefaultDict)
					| (loadDefaultDict ? BuildOption::loadDefaultDict : BuildOption::none);
			}

			string spath;
			if (modelPath)
			{
				spath = modelPath;
			}
			else
			{
				py::UniqueObj modelModule{ PyImport_ImportModule("kiwipiepy_model") };
				if (!modelModule) throw py::ExcPropagation{};
				py::UniqueObj pathFunc{ PyObject_GetAttrString(modelModule, "get_model_path")};
				if (!pathFunc) throw py::ExcPropagation{};
				py::UniqueObj pathRet{ PyObject_CallObject(pathFunc, nullptr) };
				if (!pathRet) throw py::ExcPropagation{};
				spath = py::toCpp<string>(pathRet);
			}

			self->builder = KiwiBuilder{ spath, numThreads, (BuildOption)boptions };
			return 0;
		});
	}

	void doPrepare()
	{
		if (kiwi.ready()) return;
		kiwi = builder.build();
	}

	PyObject* addUserWord(PyObject* args, PyObject* kwargs);
	PyObject* analyze(PyObject* args, PyObject* kwargs);
	PyObject* extractAddWords(PyObject* args, PyObject* kwargs);
	PyObject* extractFilterWords(PyObject* args, PyObject* kwargs);
	PyObject* extractWords(PyObject* args, PyObject* kwargs);
	PyObject* loadUserDictionary(PyObject* args, PyObject* kwargs);
	PyObject* perform(PyObject* args, PyObject* kwargs);
	PyObject* prepare(PyObject* args, PyObject* kwargs);
	PyObject* setCutOffThreshold2(PyObject* args, PyObject* kwargs);
	PyObject* get_option(PyObject* args, PyObject* kwargs);
	PyObject* set_option(PyObject* args, PyObject* kwargs);
	PyObject* getMorpheme(PyObject* args, PyObject* kwargs);
	py::UniqueObj version();

	float getCutOffThreshold() const
	{
		return kiwi.getCutOffThreshold();
	}

	void setCutOffThreshold(float v)
	{
		kiwi.setCutOffThreshold(v);
	}

	bool getIntegrateAllomorph() const
	{
		return kiwi.getIntegrateAllomorph();
	}

	void setIntegrateAllomorph(bool v)
	{
		kiwi.setIntegrateAllomorph(v);
	}

	size_t getNumWorkers() const
	{
		return kiwi.getNumThreads();
	}
};


static PyMethodDef Kiwi_methods[] =
{
	{ "add_user_word", PY_METHOD_MEMFN(&KiwiObject::addUserWord), METH_VARARGS | METH_KEYWORDS, Kiwi_add_user_word__doc__ },
	{ "load_user_dictionary", PY_METHOD_MEMFN(&KiwiObject::loadUserDictionary), METH_VARARGS | METH_KEYWORDS, Kiwi_load_user_dictionary__doc__ },
	{ "extract_words", PY_METHOD_MEMFN(&KiwiObject::extractWords), METH_VARARGS | METH_KEYWORDS, Kiwi_extract_words__doc__ },
	{ "extract_filter_words", PY_METHOD_MEMFN(&KiwiObject::extractFilterWords), METH_VARARGS | METH_KEYWORDS, Kiwi_extract_filter_words__doc__ },
	{ "extract_add_words", PY_METHOD_MEMFN(&KiwiObject::extractAddWords), METH_VARARGS | METH_KEYWORDS, Kiwi_extract_add_words__doc__ },
	{ "perform", PY_METHOD_MEMFN(&KiwiObject::perform), METH_VARARGS | METH_KEYWORDS, Kiwi_perform__doc__ },
	{ "set_cutoff_threshold", PY_METHOD_MEMFN(&KiwiObject::setCutOffThreshold2), METH_VARARGS | METH_KEYWORDS, Kiwi_set_cutoff_threshold__doc__ },
	{ "prepare", PY_METHOD_MEMFN(&KiwiObject::prepare), METH_VARARGS | METH_KEYWORDS, Kiwi_prepare__doc__ },
	{ "analyze", PY_METHOD_MEMFN(&KiwiObject::analyze), METH_VARARGS | METH_KEYWORDS, Kiwi_analyze__doc__ },
	{ "get_option", PY_METHOD_MEMFN(&KiwiObject::get_option), METH_VARARGS | METH_KEYWORDS, Kiwi_get_option__doc__ },
	{ "set_option", PY_METHOD_MEMFN(&KiwiObject::set_option), METH_VARARGS | METH_KEYWORDS, Kiwi_set_option__doc__ },
	{ "morpheme", PY_METHOD_MEMFN(&KiwiObject::getMorpheme), METH_VARARGS | METH_KEYWORDS, "" },
	{ nullptr }
};

static PyGetSetDef Kiwi_getsets[] = 
{
	{ (char*)"version", PY_GETTER_MEMFN(&KiwiObject::version), nullptr, "get version", nullptr },
	{ (char*)"cutoff_threshold", PY_GETTER_MEMFN(&KiwiObject::getCutOffThreshold), PY_SETTER_MEMFN(&KiwiObject::setCutOffThreshold), "", nullptr },
	{ (char*)"integrate_allomorph", PY_GETTER_MEMFN(&KiwiObject::getIntegrateAllomorph), PY_SETTER_MEMFN(&KiwiObject::setIntegrateAllomorph), "", nullptr },
	{ (char*)"num_workers", PY_GETTER_MEMFN(&KiwiObject::getNumWorkers), nullptr, "", nullptr },
	{ nullptr },
};

static PyTypeObject Kiwi_type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"kiwipiepy.Kiwi",             /* tp_name */
	sizeof(KiwiObject), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)KiwiObject::dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,   /* tp_flags */
	Kiwi__doc__,           /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	Kiwi_methods,             /* tp_methods */
	0,						 /* tp_members */
	Kiwi_getsets,        /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)KiwiObject::init,      /* tp_init */
	PyType_GenericAlloc,
	(newfunc)KiwiObject::_new,
};

struct KiwiTokenObject : py::CObject<KiwiTokenObject>
{
	u16string _form;
	const char* _tag = nullptr;
	uint32_t _pos = 0, _len = 0;
	size_t _morphId = 0;
	const Morpheme* _morph = nullptr;

	static int init(KiwiTokenObject* self, PyObject* args, PyObject* kwargs)
	{
		return py::handleExc([&]()
		{
			new (&self->_form) u16string;
			return 0;
		});
	}

	uint32_t end()
	{
		return _pos + _len;
	}

	static Py_ssize_t len(KiwiTokenObject* self)
	{
		return 4;
	}

	static PyObject* getitem(KiwiTokenObject* self, Py_ssize_t idx)
	{
		return py::handleExc([&]()
		{
			if (idx < 0) idx += len(self);
			switch (idx)
			{
			case 0: return py::buildPyValue(self->_form);
			case 1: return py::buildPyValue(self->_tag);
			case 2: return py::buildPyValue(self->_pos);
			case 3: return py::buildPyValue(self->_len);
			}
			throw py::IndexError{ "index out of range" };
		});
	}

	static PyObject* repr(KiwiTokenObject* self)
	{
		return py::handleExc([&]()
		{
			return py::buildPyValue("Token("
				"form=" + py::reprFromCpp(self->_form) + ", "
				"tag=" + py::reprFromCpp(self->_tag) + ", "
				"start=" + to_string(self->_pos) + ", "
				"len=" + to_string(self->_len) +
			")");
		});
	}
};


static PyGetSetDef KiwiToken_getsets[] =
{
	{ (char*)"form", PY_GETTER_MEMPTR(&KiwiTokenObject::_form), nullptr, "", nullptr },
	{ (char*)"tag", PY_GETTER_MEMPTR(&KiwiTokenObject::_tag), nullptr, "", nullptr },
	{ (char*)"start", PY_GETTER_MEMPTR(&KiwiTokenObject::_pos), nullptr, "", nullptr },
	{ (char*)"len", PY_GETTER_MEMPTR(&KiwiTokenObject::_len), nullptr, "", nullptr },
	{ (char*)"end", PY_GETTER_MEMFN(&KiwiTokenObject::end), nullptr, "", nullptr },
	{ (char*)"id", PY_GETTER_MEMPTR(&KiwiTokenObject::_morphId), nullptr, "", nullptr },
	{ nullptr },
};

static PySequenceMethods KiwiToken_seqs = {
	(lenfunc)KiwiTokenObject::len,
	nullptr,
	nullptr,
	(ssizeargfunc)KiwiTokenObject::getitem,
};

static PyTypeObject KiwiToken_type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"kiwipiepy.KiwiToken",             /* tp_name */
	sizeof(KiwiTokenObject), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)KiwiTokenObject::dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	(reprfunc)KiwiTokenObject::repr,                         /* tp_repr */
	0,                         /* tp_as_number */
	&KiwiToken_seqs,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,   /* tp_flags */
	Kiwi__doc__,           /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	0,             /* tp_methods */
	0,						 /* tp_members */
	KiwiToken_getsets,        /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)KiwiTokenObject::init,      /* tp_init */
	PyType_GenericAlloc,
	(newfunc)KiwiTokenObject::_new,
};

PyObject* resToPyList(vector<TokenResult>&& res, const Kiwi& kiwi)
{
	py::UniqueObj retList{ PyList_New(res.size()) };
	size_t idx = 0;
	for (auto& p : res)
	{
		py::UniqueObj rList{ PyList_New(p.first.size()) };
		size_t jdx = 0;
		size_t u32offset = 0;
		for (auto& q : p.first)
		{
			size_t u32chrs = 0;
			for (auto u : q.str)
			{
				if ((u & 0xFC00) == 0xD800) u32chrs++;
			}

			py::UniqueObj item{ PyObject_CallFunctionObjArgs((PyObject*)&KiwiToken_type, nullptr) };
			auto* tItem = (KiwiTokenObject*)item.get();
			tItem->_form = move(q.str);
			tItem->_tag = tagToString(q.tag);
			tItem->_pos = q.position - u32offset;
			tItem->_len = q.length - u32chrs;
			tItem->_morph = q.morph;
			tItem->_morphId = kiwi.morphToId(q.morph);

			PyList_SetItem(rList, jdx++, item.release());
			u32offset += u32chrs;
		}
		PyList_SetItem(retList, idx++, py::buildPyTuple(move(rList), p.second));
	}
	return retList.release();
}

struct KiwiResIter : public py::ResultIter<KiwiResIter, vector<TokenResult>>
{
	py::UniqueCObj<KiwiObject> kiwi;
	size_t topN = 1;
	Match matchOptions = Match::all;

	PyObject* buildPy(vector<TokenResult>&& v)
	{
		return py::handleExc([&]() -> PyObject*
		{
			if (v.size() > topN) v.erase(v.begin() + topN, v.end());
			return resToPyList(move(v), kiwi->kiwi);
		});
	}

	future<vector<TokenResult>> feedNext(py::SharedObj&& next)
	{
		if (!PyUnicode_Check(next)) throw py::ValueError{ "`analyze` requires an instance of `str` or an iterable of `str`." };
		return kiwi->kiwi.asyncAnalyze(PyUnicode_AsUTF8(next), topN, matchOptions);
	}
};

static PyTypeObject KiwiResIter_type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"kiwipiepy._res_iter",             /* tp_name */
	sizeof(KiwiResIter), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)KiwiResIter::dealloc, /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_as_async */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,   /* tp_flags */
	"",           /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	(getiterfunc)KiwiResIter::iter,                         /* tp_iter */
	(iternextfunc)KiwiResIter::iternext,                         /* tp_iternext */
	0,             /* tp_methods */
	0,						 /* tp_members */
	0,        /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)KiwiResIter::init,      /* tp_init */
	PyType_GenericAlloc,
	(newfunc)KiwiResIter::_new,
};


PyObject* KiwiObject::addUserWord(PyObject* args, PyObject *kwargs)
{	
	return py::handleExc([&]() -> PyObject*
	{
		const char* word;
		const char* tag = "NNP";
		float score = 0;
		static const char* kwlist[] = { "word", "tag", "score", nullptr };
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|sf", (char**)kwlist, &word, &tag, &score)) return nullptr;

		auto pos = toPOSTag(utf8To16(tag));
		if (pos >= POSTag::max) throw py::ValueError{ "Unknown tag value " + py::reprFromCpp(tag) };
		auto ret = builder.addWord(utf8To16(word), pos, score);
		if (ret) kiwi = Kiwi{};
		return py::buildPyValue(ret);
	});
}

PyObject* KiwiObject::loadUserDictionary(PyObject* args, PyObject *kwargs)
{
	return py::handleExc([&]() -> PyObject*
	{
		const char* path;
		static const char* kwlist[] = { "dict_path", nullptr };
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", (char**)kwlist, &path)) return nullptr;

		auto ret = builder.loadDictionary(path);
		if (ret) kiwi = Kiwi{};
		return py::buildPyValue(ret);
	});
}

U16MultipleReader obj2reader(PyObject* obj)
{
	return [obj]()
	{
		py::SharedObj iter{ PyObject_GetIter(obj) };

		if (!iter) throw py::ExcPropagation{};
		return [iter]() -> u16string
		{
			py::UniqueObj item{ PyIter_Next(iter) };
			if (!item)
			{
				if (PyErr_Occurred()) throw py::ExcPropagation{};
				return {};
			}
			auto ret = py::toCpp<u16string>(item);
			if (ret.empty()) ret.push_back(' ');
			return ret;
		};
	};
}

PyObject* KiwiObject::extractWords(PyObject* args, PyObject *kwargs)
{
	return py::handleExc([&]() -> PyObject*
	{
		PyObject* sentences;
		size_t minCnt = 10, maxWordLen = 10;
		float minScore = 0.25f, posThreshold = -3;
		size_t lmFilter = 1;
		static const char* kwlist[] = { "sentences", "min_cnt", "max_word_len", "min_score", "pos_threshold", "lm_filter", nullptr };
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|nnffp", (char**)kwlist, &sentences, &minCnt, &maxWordLen, &minScore, &posThreshold, &lmFilter)) return nullptr;

		auto res = builder.extractWords(obj2reader(sentences), minCnt, maxWordLen, minScore, posThreshold, lmFilter);

		py::UniqueObj retList{ PyList_New(res.size()) };
		size_t idx = 0;
		for (auto& r : res)
		{
			auto v = py::buildPyTuple(utf16To8(r.form).c_str(), r.score, r.freq, r.posScore[POSTag::nnp]);
			if (!v) throw py::ExcPropagation{};
			PyList_SetItem(retList, idx++, v);
		}
		return retList.release();
	});
}

PyObject* KiwiObject::extractFilterWords(PyObject* args, PyObject *kwargs)
{
	if (PyErr_WarnEx(PyExc_DeprecationWarning, "`extract_filter_words` has same effect to `extract_words` and will be removed in future version.", 1)) return nullptr;
	return extractWords(args, kwargs);
}

PyObject* KiwiObject::extractAddWords(PyObject* args, PyObject *kwargs)
{
	return py::handleExc([&]() -> PyObject*
	{
		PyObject* sentences;
		size_t minCnt = 10, maxWordLen = 10;
		float minScore = 0.25f, posScore = -3;
		size_t lmFilter = 1;
		static const char* kwlist[] = { "sentences", "min_cnt", "max_word_len", "min_score", "pos_score", "lm_filter", nullptr };
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|nnffp", (char**)kwlist, &sentences, &minCnt, &maxWordLen, &minScore, &posScore, &lmFilter)) return nullptr;

		auto res = builder.extractAddWords(obj2reader(sentences), minCnt, maxWordLen, minScore, posScore, lmFilter);
		py::UniqueObj retList{ PyList_New(res.size()) };
		size_t idx = 0;
		for (auto& r : res)
		{
			auto v = py::buildPyTuple(utf16To8(r.form).c_str(), r.score, r.freq, r.posScore[POSTag::nnp]);
			if (!v) throw py::ExcPropagation{};
			PyList_SetItem(retList, idx++, v);
		}
		return retList.release();
	});
}

PyObject* KiwiObject::setCutOffThreshold2(PyObject* args, PyObject *kwargs)
{
	return py::handleExc([&]() -> PyObject*
	{
		float threshold;
		static const char* kwlist[] = { "threshold", nullptr };
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "f", (char**)kwlist, &threshold)) return nullptr;

		if (PyErr_WarnEx(PyExc_DeprecationWarning, "`set_cutoff_threshold(v)` will be removed in future version. Use `Kiwi.cutoff_threshold = v` instead.", 1)) return nullptr;
		kiwi.setCutOffThreshold(threshold);
		Py_INCREF(Py_None);
		return Py_None;
	});
}

PyObject* KiwiObject::prepare(PyObject* args, PyObject *kwargs)
{
	return py::handleExc([&]() -> PyObject*
	{
		static const char* kwlist[] = { nullptr };
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", (char**)kwlist)) return nullptr;

		if (PyErr_WarnEx(PyExc_DeprecationWarning, "`prepare()` has no effect and will be removed in future version.", 1)) return nullptr;
		Py_INCREF(Py_None);
		return Py_None;
	});
}

PyObject* KiwiObject::get_option(PyObject* args, PyObject *kwargs)
{
	return py::handleExc([&]() -> PyObject*
	{
		size_t option;
		static const char* kwlist[] = { "option", nullptr };
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "n", (char**)kwlist, &option)) return nullptr;

		if (PyErr_WarnEx(PyExc_DeprecationWarning, "`get_option()` will be removed in future version.", 1)) return nullptr;
		return py::buildPyValue(kiwi.getIntegrateAllomorph());
	});
}

PyObject* KiwiObject::set_option(PyObject* args, PyObject *kwargs)
{
	return py::handleExc([&]() -> PyObject*
	{
		size_t option, value;
		static const char* kwlist[] = { "option", "value", nullptr };
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "nn", (char**)kwlist, &option, &value)) return nullptr;

		if (PyErr_WarnEx(PyExc_DeprecationWarning, "`set_option()` will be removed in future version.", 1)) return nullptr;
		kiwi.setIntegrateAllomorph(value);
		Py_INCREF(Py_None);
		return Py_None;
	});
}

PyObject* KiwiObject::analyze(PyObject* args, PyObject *kwargs)
{
	return py::handleExc([&]() -> PyObject*
	{
		size_t topN = 1, matchOptions = (size_t)Match::all;
		PyObject* text;
		static const char* kwlist[] = { "text", "top_n", "match_options", nullptr };
		if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|nn", (char**)kwlist, &text, &topN, &matchOptions)) return nullptr;

		doPrepare();
		if (PyUnicode_Check(text))
		{
			auto res = kiwi.analyze(PyUnicode_AsUTF8(text), max(topN, (size_t)10), (Match)matchOptions);
			if (res.size() > topN) res.erase(res.begin() + topN, res.end());
			return resToPyList(move(res), kiwi);
		}
		else
		{
			py::UniqueObj iter{ PyObject_GetIter(text) };
			if (!iter) throw py::ValueError{ "`analyze` requires a `str` or an iterable of `str` parameters." };
			py::UniqueCObj<KiwiResIter> ret{ (KiwiResIter*)PyObject_CallObject((PyObject*)&KiwiResIter_type, nullptr) };
			if (!ret) throw py::ExcPropagation{};
			ret->kiwi = py::UniqueCObj<KiwiObject>{ this };
			Py_INCREF(this);
			ret->inputIter = move(iter);
			ret->topN = topN;
			ret->matchOptions = (Match)matchOptions;
			for (int i = 0; i < kiwi.getNumThreads() * 16; ++i)
			{
				if (!ret->feed()) break;
			}
			return (PyObject*)ret.release();
		}
	});
}

PyObject* KiwiObject::perform(PyObject* args, PyObject *kwargs)
{
	size_t topN = 1, matchOptions = (size_t)Match::all;
	PyObject* sentences;
	size_t minCnt = 10, maxWordLen = 10;
	float minScore = 0.25f, posScore = -3;
	size_t lmFilter = 1;
	static const char* kwlist[] = { "sentences", "top_n", "match_options", "min_cnt", "max_word_len", "min_score", "pos_score", "lm_filter", nullptr};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|nnnnffp", (char**)kwlist, 
		&sentences, &topN, &matchOptions, &minCnt, &maxWordLen, &minScore, &posScore, &lmFilter)) return nullptr;
	return py::handleExc([&]()
	{
		Py_INCREF(Py_None);
		return Py_None;
	});
}

PyObject* KiwiObject::getMorpheme(PyObject* args, PyObject* kwargs)
{
	size_t id = 0;
	static const char* kwlist[] = { "id", nullptr };
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "n", (char**)kwlist, &id)) return nullptr;
	return py::handleExc([&]()
	{
		py::UniqueObj ret{ PyObject_CallFunctionObjArgs((PyObject*)&KiwiToken_type, nullptr) };
		auto* obj = (KiwiTokenObject*)ret.get();
		auto* morph = kiwi.idToMorph(id);
		if (!morph) throw py::ValueError{ "out of range" };
		auto& form = morph->getForm();
		obj->_form = u16string{ form.begin(), form.end() };
		obj->_tag = tagToString(morph->tag);
		obj->_morph = morph;
		obj->_morphId = id;
		return ret.release();
	});
}

py::UniqueObj KiwiObject::version()
{
	return py::UniqueObj{ py::handleExc([&]() -> PyObject*
	{
		if (PyErr_WarnEx(PyExc_DeprecationWarning, "`version` will be removed in future version.", 1)) return nullptr;
		return py::buildPyValue("0.10.0");
	}) };
}

PyObject* moduleInit()
{
	static PyModuleDef mod =
	{
		PyModuleDef_HEAD_INIT,
		"_kiwipiepy",
		"Kiwi API for Python",
		-1,
		nullptr
	};

	gModule = PyModule_Create(&mod);

	if (PyType_Ready(&Kiwi_type) < 0) return nullptr;
	Py_INCREF(&Kiwi_type);
	PyModule_AddObject(gModule, "Kiwi", (PyObject*)&Kiwi_type);

	if (PyType_Ready(&KiwiResIter_type) < 0) return nullptr;
	Py_INCREF(&KiwiResIter_type);
	PyModule_AddObject(gModule, "_res_iter", (PyObject*)&KiwiResIter_type);

	if (PyType_Ready(&KiwiToken_type) < 0) return nullptr;
	Py_INCREF(&KiwiToken_type);
	PyModule_AddObject(gModule, "_token", (PyObject*)&KiwiToken_type);

	return gModule;
}

PyMODINIT_FUNC PyInit__kiwipiepy()
{
	return moduleInit();
}
