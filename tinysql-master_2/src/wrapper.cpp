#include <iostream>
#include <algorithm>
#include <iterator>
#include <cassert>

#include "debug.h"
#include "util.h"
#include "obj_util.h"

using tiny::DrawTable;
using tiny::is_contain;
using tiny::add_into;
using tiny::delete_not_null;
using tiny::error_msg;
using tiny::warning_msg;

#include "Block.h"
#include "Config.h"
#include "Disk.h"
#include "Field.h"
#include "MainMemory.h"
#include "Relation.h"
#include "Schema.h"
#include "SchemaManager.h"
#include "Tuple.h"

#include "dbMgr.h"
#include "wrapper.h"
#include "tiny_util.h"

//------------------------------------------------------------------------------
//   
//------------------------------------------------------------------------------
DataType field_to_data_type(const FIELD_TYPE&);
FIELD_TYPE tiny_to_field_type(const DataType&);

//------------------------------------------------------------------------------
//   
//------------------------------------------------------------------------------
string tiny_dump_str(const DataType& type)
{
    switch (type) {
        case TINY_INT:
            return "INT";
        case TINY_STR20:
            return "STR20";
        case TINY_UNKNOWN:
            return "UNKNOWN";
    }

    return "ERROR";
}

DataType field_to_data_type(const FIELD_TYPE& type)
{
    if (type == INT) {
        return TINY_INT;
    }

    assert(type == STR20);
    return TINY_STR20;
}

FIELD_TYPE tiny_to_field_type(const DataType& type)
{
    if (type == TINY_INT) {
        return INT;
    } else {
        assert(type == TINY_STR20);
        return STR20;
    }
}

//------------------------------------------------------------------------------
//   TinySchema
//------------------------------------------------------------------------------

TinySchema::TinySchema(const vector<pair<string, DataType>>& attribute_type_list)
: _schema(NULL)
{
    vector<string> field_names;
    vector<FIELD_TYPE> field_types;
    for (const auto& name_type : attribute_type_list) {
        field_names.push_back(name_type.first);
        field_types.push_back(tiny_to_field_type(name_type.second));
    }

    _schema = new Schema(field_names, field_types);
}

TinySchema::TinySchema(const Schema& s)
: _schema(NULL)
{
    _schema = new Schema(s);
}

TinySchema::TinySchema(const TinySchema& rhs)
: _schema(NULL)
{
    assign(rhs);    
}

TinySchema::TinySchema(TinySchema&& rhs)
: _schema(NULL)
{
    swap(_schema, rhs._schema);
}

TinySchema::~TinySchema()
{
    delete_not_null(_schema);
}

DataType TinySchema::get_data_type(const string& field_name) const
{
    vector<pair<string, DataType>> name_type_list = get_attr_type_list();

    for (const auto& name_type : name_type_list) {
        const string& name = name_type.first;
        if (name == field_name)
            return name_type.second;
    }
    return TINY_UNKNOWN;
}

vector<string> TinySchema::get_attr_list() const
{
    return _schema->getFieldNames();
}

vector<DataType> TinySchema::get_type_list() const
{
    vector<FIELD_TYPE> types = _schema->getFieldTypes();
    vector<DataType> tmp;
    for (const auto& type : types) {
        tmp.push_back(field_to_data_type(type));
    }
    return tmp;
}

vector<pair<string, DataType>> TinySchema::get_attr_type_list() const
{
    vector<string> names = get_attr_list();
    vector<DataType> types = get_type_list();
    assert(names.size() == types.size());

    vector<pair<string, DataType>> res = tiny::vector_make_pair(names, types);
    return res;
}

size_t TinySchema::size() const
{
    return (size_t)_schema->getNumOfFields();
}

size_t TinySchema::tuple_per_block() const
{
    return (size_t)_schema->getTuplesPerBlock();
}

bool TinySchema::is_field_name_exist(const string& name) const
{
    vector<string> attr_list = get_attr_list();
    bool res = is_contain(attr_list, name);
    return res;
}

void TinySchema::assign(const TinySchema& rhs)
{
    delete_not_null(_schema);
    _schema = new Schema( *(rhs._schema) );
}

bool TinySchema::is_equal_to(const TinySchema& rhs) const
{
    return *_schema == *(rhs._schema);
}

void TinySchema::dump() const
{
    cout << dump_str() << endl;
}

string TinySchema::dump_str() const
{
    vector<pair<string, DataType>> attr_type_list = get_attr_type_list();

    string tmp;
    tmp += "(";
    for (size_t i = 0; i < attr_type_list.size(); ++i) {
        if (i != 0) {
            tmp += ", ";
        }
        tmp += attr_type_list[i].first;
        tmp += " ";
        DataType type = attr_type_list[i].second;
        tmp += tiny_dump_str(type);
    }
    tmp += ")";

    return tmp;
}

//------------------------------------------------------------------------------
//   TinyTuple
//------------------------------------------------------------------------------
TinyTuple::TinyTuple(const Tuple& tuple)
: _tuple(NULL)
{
    _tuple = new Tuple(tuple);
}

TinyTuple::TinyTuple(const TinyTuple& rhs)
: _tuple(NULL)
{
    assign(rhs);
}

TinyTuple::TinyTuple(TinyTuple&& rhs)
: _tuple(NULL)
{
    swap(_tuple, rhs._tuple);
}

TinyTuple::~TinyTuple()
{
    delete_not_null(_tuple);
}

bool TinyTuple::set_raw_value(const string& name, const string& raw_value)
{
    DataType data_type = get_data_type(name);
    if (data_type == TINY_UNKNOWN) {
        error_msg_attribute_not_exist(name);
        return false;
    }

    bool res = false;
    if (raw_value == "NULL") {
        warning_msg("does not support 'NULL' now, set to default value");
        set_value(name, DataValue(data_type));
        res = true;
    } else if (raw_value[0] == '\"') {
        if (data_type == TINY_INT) {
            error_msg("\'" + name + "\' should be INT");
            return false;
        }
        res = set_str_value(name, get_literal_value(raw_value));
    } else {
        if (data_type != TINY_INT) {
            assert(data_type == TINY_STR20);
            error_msg("\'" + name + "\' should be STR20");
            return false;
        }
        res = set_int_value(name, tiny::str_to<int>(raw_value));
    }

    return res;
}

void TinyTuple::set_value(const string& name, const DataValue& data_value)
{
    if (data_value.get_type() == TINY_INT) {
        set_int_value(name, data_value.get_int());
    } else {
        assert(data_value.get_type() == TINY_STR20);
        set_str_value(name, data_value.get_str());
    }
}

void TinyTuple::set_value(size_t i, const DataValue& type_value)
{
    const DataType& type = type_value.get_type(); 
    if (type == TINY_INT) {
        _tuple->setField(i, type_value.get_int());
    } else {
        assert(type == TINY_STR20);

        _tuple->setField(i, type_value.get_str());
    }
}

void TinyTuple::set_value(const TinyTuple& t1, const TinyTuple& t2)
{
    assert(size() == t1.size() + t2.size()); 
    assert(!t1.is_null());
    assert(!t2.is_null());
    
    vector<DataValue> tmp;
    add_into(tmp, t1.get_value_list());
    add_into(tmp, t2.get_value_list());
     
    for (size_t i = 0; i < tmp.size(); ++i) {
        set_value(i, tmp[i]);
    }
}

bool TinyTuple::set_str_value(const string& name, const string& value)
{
    bool res = _tuple->setField(name, value);
    return res;
}

bool TinyTuple::set_int_value(const string& name, int value)
{
    bool res = _tuple->setField(name, value);
    return res;
}

void TinyTuple::init()
{
    vector<DataType> type_list = get_type_list();
    for (size_t i = 0; i < type_list.size(); ++i) {
        set_value(i, DataValue(type_list[i]));
    }
}

void TinyTuple::set_null()
{
    _tuple->null();
}

TinySchema TinyTuple::get_tiny_schema() const
{
    return TinySchema(_tuple->getSchema());
}

size_t TinyTuple::size() const
{
    return get_tiny_schema().size();
}

DataType TinyTuple::get_data_type(const string& name) const
{
    return get_tiny_schema().get_data_type(name);
}

DataValue TinyTuple::get_data_value(const string& name) const
{
    assert(_tuple != NULL);

    DataType type = get_data_type(name);

    DataValue res;

    if (type == TINY_INT) {
        Field value = _tuple->getField(name);
        res = value.integer;
    } else if (type == TINY_STR20) {
        Field value = _tuple->getField(name);
        res = *(value.str);
    }

    return res;
}

vector<pair<string, DataType>> TinyTuple::get_attr_type_list() const
{
    return get_tiny_schema().get_attr_type_list();
}

vector<string> TinyTuple::get_attr_list() const
{
    return get_tiny_schema().get_attr_list();
}

vector<DataType> TinyTuple::get_type_list() const
{
    return get_tiny_schema().get_type_list();
}

vector<DataValue> TinyTuple::get_value_list() const
{
    vector<DataType> type_list = get_type_list();

    vector<DataValue> res;
    for (size_t i = 0; i < type_list.size(); ++i) {
        Field value = _tuple->getField(i);
        if (type_list[i] == TINY_INT) {

            res.push_back(DataValue(value.integer)); 
        } else {
            assert(type_list[i] == TINY_STR20);
    
            res.push_back(DataValue(*(value.str))); 
        }
    }

    return res;
}

vector<string> TinyTuple::get_str_list() const
{
    vector<DataValue> value_list = get_value_list();
    vector<string> res;
    for (const auto& value : value_list) {
        res.push_back(value.dump_str());
    }
    return res;
}

bool TinyTuple::is_null() const
{   
    assert(_tuple != NULL);
    
    return _tuple->isNull();
}

bool TinyTuple::is_attr_exist(const string& attr) const
{
    assert(_tuple != NULL);
    
    bool res = get_tiny_schema().is_field_name_exist(attr);
    return res;
}

void TinyTuple::assign(const TinyTuple& rhs)
{
    delete_not_null(_tuple);
    _tuple = new Tuple(rhs);
}

bool TinyTuple::is_less_than(const TinyTuple& rhs) const
{
    bool res = get_value_list() < rhs.get_value_list();;
    return res;
}

bool TinyTuple::is_equal_to(const TinyTuple& rhs) const
{
    bool res = get_value_list() == rhs.get_value_list();;
    return res;
}

bool TinyTuple::is_less_than_by_attr(const TinyTuple& rhs, const vector<string>& attr_list) const
{
    for (const string& attr : attr_list) {
        DataValue value = get_data_value(attr);
        DataValue rhs_value = rhs.get_data_value(attr);

        if (value < rhs_value) {
            return true;
        } else if (value > rhs_value) {
            return false;
        }
    }

    return is_less_than(rhs);
}

string TinyTuple::dump_str() const
{
    if (is_null())
        return "<DELETE>";

    vector<pair<string, DataType>> name_type_list = get_tiny_schema().get_attr_type_list();

    string tmp = "(";
    for (size_t i = 0; i < name_type_list.size(); ++i) {
        if (i != 0) {
            tmp += ", ";
        }
        const auto& name = name_type_list[i].first;
        const auto& type = name_type_list[i].second;
        Field value = _tuple->getField(name);
        if (type == TINY_INT) {
            tmp += name;    
            tmp += " ";
            tmp += tiny::dump_str(value.integer);
        } else {
            assert(type == TINY_STR20);

            tmp += name;    
            tmp += " ";
            tmp += *(value.str);
        }
    }

    tmp += ")";
    return tmp;
}

void TinyTuple::dump() const
{
    cout << dump_str() << endl;
}

//------------------------------------------------------------------------------
//   
//------------------------------------------------------------------------------
TinyBlock::TinyBlock(Block* block)
: _block(block)
{
    ;
}

vector<Tuple> TinyBlock::get_tuples() const
{
    return _block->getTuples();
}

vector<TinyTuple> TinyBlock::get_tiny_tuples() const
{
    vector<TinyTuple> tmp;

    vector<Tuple> tuples = get_tuples();
    for (const auto& tuple : tuples) {
        tmp.push_back(tuple);
    }

    return tmp;
}

TinyTuple TinyBlock::get_tiny_tuple(size_t i) const
{
    return _block->getTuple(i);
}

size_t TinyBlock::size()
{
    return get_tuples().size();
}

void TinyBlock::push_back(const TinyTuple& tuple)
{
    _block->appendTuple(tuple);
}

void TinyBlock::push_back(const vector<TinyTuple>& tuples)
{
    for (const auto& tuple : tuples) {
        push_back(tuple);
    }
}

void TinyBlock::set_tuple(size_t i, const TinyTuple& tuple)
{
    _block->setTuple(i, tuple);
}

void TinyBlock::remove_null_tuple()
{
    vector<Tuple> tuples = get_tuples();
    _block->clear();
    for (const auto& tuple : tuples) {
        if (!tuple.isNull()) {
            push_back(tuple);
        }
    }
}

void TinyBlock::null_tuple(size_t i)
{
    _block->nullTuple(i);
}

void TinyBlock::clear()
{
    _block->clear();
}

bool TinyBlock::empty() const
{
    vector<Tuple> tuples = get_tuples();
    for (const auto& tuple : tuples) {
        if (!tuple.isNull())
            return false;
    }
    return true;
}

void TinyBlock::dump() const
{
    vector<TinyTuple> tuples = get_tiny_tuples();
    for (const auto& tuple : tuples) {
        tuple.dump();
    }
}

string TinyBlock::dump_str() const
{
    return "TODO";
}

vector<string> TinyBlock::dump_str_list() const
{
    size_t count = 0;
    vector<TinyTuple> tuples = get_tiny_tuples();

    vector<string> res;
    for (const auto& tuple : tuples) {
        if (tuple.is_null()) {
            size_t num = tuple.size();
            count += num;
            for (size_t i = 0; i < num; ++i) {
                res.push_back(""); 
            }
        } else {
            const vector<string> str_list = tuple.get_str_list();
            count += str_list.size();
            add_into(res, str_list); 
        }
    }

    for (size_t i = count; i < HwMgr::ins()->get_block_size(); ++i) {
        res.push_back("");
    }
    return res;
}

//------------------------------------------------------------------------------
//   
//------------------------------------------------------------------------------
MemIter::MemIter(const TinyRelation* r, size_t pos)
: _relation(r)
, _pos(pos)
{
    ;
}

void MemIter::set_tuple(const TinyTuple& tuple)
{
    size_t block_idx = get_block_idx();
    size_t tuple_idx = get_tuple_idx();
    TinyBlock block = HwMgr::ins()->get_mem_block(block_idx);
    block.set_tuple(tuple_idx, tuple);
}

TinyTuple MemIter::get_tuple() const
{
    size_t block_idx = get_block_idx();
    size_t tuple_idx = get_tuple_idx();
    TinyBlock block = HwMgr::ins()->get_mem_block(block_idx);
    return block.get_tiny_tuple(tuple_idx);
}

size_t MemIter::get_block_idx() const
{
    return _pos / _relation->tuple_per_block();
}

size_t MemIter::get_tuple_idx() const
{
    return _relation->get_tuple_idx_by_pos(_pos);
}

bool MemIter::is_equal_to(const MemIter& rhs) const
{
    assert(_relation == rhs._relation);

    bool res = _pos == rhs._pos;
    return res;
}

string MemIter::dump_str() const
{
    return tiny_dump_str(get_block_idx(), get_tuple_idx());
}
//------------------------------------------------------------------------------
//   RelIter
//------------------------------------------------------------------------------
RelIter::RelIter(const TinyRelation* r)
: _relation(r)
, _pos(0)
{
    ;
}

RelIter::RelIter(const TinyRelation* r, size_t pos)
: _relation(r)
, _pos(pos)
{
    ;
}

bool RelIter::is_equal_to(const RelIter& rhs) const
{
    assert(_relation == rhs._relation);

    bool res = _pos == rhs._pos;
    return res;
}

bool RelIter::is_greater_than(const RelIter& rhs) const
{
    assert(_relation == rhs._relation);

    bool res = _pos > rhs._pos;
    return res;
}

size_t RelIter::get_block_idx() const
{
    return _relation->get_block_idx_by_pos(_pos);
}

size_t RelIter::get_tuple_idx() const
{
    return _relation->get_tuple_idx_by_pos(_pos);
}

TinyTuple RelIter::load_to_mem(size_t mem_idx) const
{
    pair<size_t, size_t> block_tuple_idx = _relation->get_idx_by_pos(_pos);
    size_t block_idx = block_tuple_idx.first;
    size_t tuple_idx = block_tuple_idx.second;
    _relation->load_block_to_mem(block_idx, mem_idx);
    TinyBlock block = HwMgr::ins()->get_mem_block(mem_idx);
    return  block.get_tiny_tuple(tuple_idx);
}

TinyTuple RelIter::get_from_mem(size_t mem_idx) const
{
    pair<size_t, size_t> block_tuple_idx = _relation->get_idx_by_pos(_pos);
    size_t tuple_idx = block_tuple_idx.second;
    TinyBlock block = HwMgr::ins()->get_mem_block(mem_idx);
    return  block.get_tiny_tuple(tuple_idx);
}

void RelIter::skip_null()
{
    while (_relation->end() != *this && is_null()) {
        ++_pos;
    }
}

bool RelIter::is_null() const
{
    return _relation->is_null(_pos);
}

string RelIter::dump_str() const
{
    return tiny_dump_str(get_block_idx(), get_tuple_idx());
}

//------------------------------------------------------------------------------
//   
//------------------------------------------------------------------------------
RelRange::RelRange(const RelIter& begin, const RelIter& end)
: _begin(begin)
, _end(end)
{
    assert(_begin <= _end);
}

size_t RelRange::num_of_block() const
{
    size_t res = _end.get_block_idx() - _begin.get_block_idx() + 1;
    return res;
}

bool RelRange::is_equal_to(const RelRange& rhs) const
{
    return _begin == rhs._begin && _end == rhs._end;
}

string RelRange::dump_str() const
{
    return tiny_dump_str(_begin, _end);
}

//------------------------------------------------------------------------------
//   
//------------------------------------------------------------------------------
RelWriter::RelWriter(TinyRelation* r)
: _relation(r)
//, _mem_idx(0)
{
    assert(_relation != NULL);
}

RelWriter::~RelWriter()
{
    assert(_tuples.empty());
}

void RelWriter::push_back(const TinyTuple& tuple)
{
    _tuples.push_back(tuple);
    if (_tuples.size() == _relation->get_last_block_capacity()) {
        _relation->push_back_block(_tuples);
        _tuples.clear();
    }
}

void RelWriter::flush()
{
    if (!_tuples.empty()) {
        _relation->push_back_block(_tuples);
        _tuples.clear();
    }
}

//------------------------------------------------------------------------------
//   
//------------------------------------------------------------------------------
RelScanner::RelScanner(const TinyRelation* r, size_t base_idx, size_t mem_size)
: _relation(r)
, _base_idx(base_idx)
, _mem_size(mem_size)
, _m_iter(r, 0)
, _m_iter_end(r, 0)
, _iter(r->begin())
, _iter_end(r->end())
{
    assert(_relation != NULL);
    assert(_base_idx >= 0 && _base_idx < HwMgr::ins()->get_mem_size());
    assert(_mem_size >= 1);
    assert(_base_idx + _mem_size - 1 < HwMgr::ins()->get_mem_size());
}

RelScanner::RelScanner(const TinyRelation* r, const MemRange& range)
: _relation(r)
, _base_idx(range.get_base_idx())
, _mem_size(range.size())
, _m_iter(r, 0)
, _m_iter_end(r, 0)
, _iter(r->begin())
, _iter_end(r->end())
{
    assert(_relation != NULL);
    assert(_base_idx >= 0 && _base_idx < HwMgr::ins()->get_mem_size());
    assert(_mem_size >= 1);
    assert(_base_idx + _mem_size - 1 < HwMgr::ins()->get_mem_size());
}

void RelScanner::set_range(const RelRange& range)
{
    _iter = range.begin();
    _iter_end = range.end();
}

TinyTuple RelScanner::get_next()
{
    if (_m_iter == _m_iter_end) {
        _iter.skip_null();
        if (is_iter_end()) {
            return _relation->create_null_tuple();
        } else {
            load_to_mem();
        }
    }

    assert(_m_iter != _m_iter_end);
    TinyTuple res = _m_iter.get_tuple();
    ++_m_iter;

    return res;
}

TinyTuple RelScanner::peep_next() // TODO
{
    if (_m_iter == _m_iter_end) {
        _iter.skip_null();
        if (is_iter_end()) {
            return _relation->create_null_tuple();
        } else {
            load_to_mem();
        }
    }

    assert(_m_iter != _m_iter_end);
    TinyTuple res = _m_iter.get_tuple();

    return res;
}

bool RelScanner::sort(const vector<string>& attr_list)
{
    vector<TinyTuple> tuple_list;
    for (MemIter it = _m_iter; it != _m_iter_end; ++it) {
        tuple_list.push_back(it.get_tuple());
    }

    vector<string> search_name_list = _relation->get_attr_search_name_list(attr_list);

    std::sort(tuple_list.begin(), tuple_list.end(),
        [&](const TinyTuple& a, const TinyTuple& b) -> bool
    { 
        return a.is_less_than_by_attr(b, search_name_list);
    });

    size_t i = 0;
    for (MemIter it = _m_iter; it != _m_iter_end; ++it) {
        it.set_tuple(tuple_list[i]);
        ++i;
    }

    return true;
}

void RelScanner::clear_mem()
{
    for (size_t i = 0; i < _mem_size; ++i) {
        size_t mem_idx = _base_idx + i;
        clear_mem_block(mem_idx);
    }
}

void RelScanner::clear_mem_block(size_t i)
{
    assert(i >= _base_idx);
    assert(i < _base_idx + _mem_size);

    HwMgr::ins()->get_mem_block(i).clear();
}

void RelScanner::load_to_mem()
{
    clear_mem();

    _m_iter = m_begin();
    bool reuse_block = false;
    size_t old_block_idx = 0;
    size_t load_idx = get_last_mem_block();
    for (_m_iter = m_begin(); _m_iter != m_load_end(); ++_m_iter) {
        _iter.skip_null();
        if (is_iter_end()) {
            break;
        }

        size_t block_idx = _iter.get_block_idx();
        TinyTuple tuple = _relation->create_null_tuple();
        if (reuse_block && block_idx == old_block_idx) {
            tuple = _iter.get_from_mem(load_idx); 
        } else {
            reuse_block = true;
            old_block_idx = block_idx;
            tuple = _iter.load_to_mem(load_idx); 
        }
        ++_iter;

        _m_iter.set_tuple(tuple);
    }

    if (_m_iter == m_load_end()) {
        _iter.skip_null();
        if (!is_iter_end()) {
            const size_t last_block_idx = _iter.get_block_idx();
            const size_t first_tuple_idx = _iter.get_tuple_idx();

            _relation->load_block_to_mem(last_block_idx, load_idx);
            TinyBlock block = HwMgr::ins()->get_mem_block(load_idx);

            // remove front
            for (size_t i = 0; i < first_tuple_idx; ++i) {
                block.null_tuple(i);
            }

            while (!is_iter_end() && _iter.get_block_idx() == last_block_idx) {
                ++_iter;
            }

            // remove back
            if (_iter.get_block_idx() == last_block_idx) {
                const size_t end_tuple_idx = _iter.get_tuple_idx();
                for (size_t i = end_tuple_idx; i < block.size(); ++i) {
                    block.null_tuple(i);
                }
            }

            block.remove_null_tuple();
            //block.dump();
            for (size_t i = 0; i < block.size(); ++i) {
                ++_m_iter;
            }

        }
    }

    _m_iter_end = _m_iter;
    _m_iter = m_begin();
    assert(_m_iter != _m_iter_end);
}

void RelScanner::add_mem_into(TinyRelation& new_relation) const
{
    RelWriter writer(&new_relation);
    for (MemIter it = _m_iter; it != _m_iter_end; ++it) {
        writer.push_back(it.get_tuple());
    }
    writer.flush();
}

bool RelScanner::is_iter_end() const
{
    bool res = _iter == _iter_end;
    return res;
}

bool RelScanner::is_end() const
{
    bool res = is_iter_end() && _m_iter == _m_iter_end;
    return res;
}

MemIter RelScanner::m_begin()
{
    size_t mem_pos = _base_idx * _relation->tuple_per_block();
    return MemIter(_relation, mem_pos);
}

MemIter RelScanner::m_load_end()
{
    size_t mem_pos = (_base_idx + _mem_size - 1) * _relation->tuple_per_block();
    return MemIter(_relation, mem_pos);
}

size_t RelScanner::get_last_mem_block() const
{
    return _base_idx + _mem_size - 1;
}

string RelScanner::dump_str() const
{
    string res = _relation->get_name();
    res += ": ";
    res += _m_iter.dump_str() + "/" + _m_iter_end.dump_str();
    res += " - ";
    res += _iter.dump_str() + "/" + _iter_end.dump_str();
    return res;
}

void RelScanner::dump() const
{
    using tiny::dump_impl;
    dump_normal(_base_idx);
    dump_normal(_mem_size);
    dump_normal(_m_iter);
    dump_normal(_m_iter_end);
    dump_normal(_iter);
    dump_normal(_iter_end);
}


//------------------------------------------------------------------------------
//   
//------------------------------------------------------------------------------
RelSorter::RelSorter(TinyRelation* r, size_t base_idx, size_t mem_size)
: _relation(r)
, _base_idx(base_idx)
, _mem_size(mem_size)
{
    assert(_relation != NULL);
    assert(_base_idx >= 0 && _base_idx < HwMgr::ins()->get_mem_size());
    assert(_mem_size >= 1);
    assert(_base_idx + _mem_size - 1 < HwMgr::ins()->get_mem_size());
}

void RelSorter::sort(const string& attr)
{
    assert(!attr.empty());

    _attr_list.clear();
    _attr_list.push_back(attr);    
    sort();
}

vector<pair<DataValue, RelRange>> RelSorter::sort_return(const string& attr)
{
    assert(!attr.empty());

    _attr_list.clear();
    _attr_list.push_back(attr);    
    return sort_return();
}

void RelSorter::sort()
{
    vector<pair<DataValue, RelRange>> res;

    if (_relation->get_num_of_block() <= _mem_size) {
        RelScanner scanner(_relation, _base_idx, _mem_size);
        scanner.load_to_mem();
        scanner.sort(_attr_list);


        _relation->clear();
        scanner.add_mem_into(*_relation);

        return;
    }

    string new_table_name = "tmp " + _relation->get_name();
    TinyRelation* new_relation = HwMgr::ins()->create_relation(
        new_table_name, _relation->get_tiny_schema());


    RelScanner scanner(_relation, _base_idx, _mem_size);
    RelIter it = new_relation->end();
    vector<pair<RelIter, RelIter>> sub_list;
    while (!scanner.is_iter_end()) {
        scanner.load_to_mem();
        scanner.sort(_attr_list);
        scanner.add_mem_into(*new_relation);

        RelIter it_end = new_relation->end();
        sub_list.push_back(make_pair(it, it_end));
        it = it_end;
    }

    while (_mem_size < sub_list.size()) {
        new_relation = reduce_sub_list(new_relation, sub_list);
    }

    vector<RelScanner> scanner_list = get_scanner_list(new_relation, sub_list);

    _relation->clear();

    if (_attr_list.size() == 1) {
        string search_name = _relation->get_attr_search_name(_attr_list[0]);

        RelIter iter_end = _relation->begin();

        RelWriter writer(_relation);
        TinyTuple tuple = get_max(scanner_list);
        DataValue old_value = tuple.get_data_value(search_name);;
        while (!tuple.is_null()) {

            DataValue value = tuple.get_data_value(search_name);
            if (value != old_value) {
                writer.flush();
                RelIter iter = _relation->end();
                res.push_back( make_pair( old_value, RelRange(iter_end, iter)) );
                old_value = value;
                iter_end = iter;
            }

            writer.push_back(tuple);
            tuple = get_max(scanner_list);
        }
        writer.flush();
        res.push_back( make_pair( old_value, RelRange(iter_end, _relation->end())) );
    } else {
        RelWriter writer(_relation);
        TinyTuple tmp = get_max(scanner_list);
        while (!tmp.is_null()) {
            writer.push_back(tmp);
            tmp = get_max(scanner_list);
        }
        writer.flush();
    }

    HwMgr::ins()->drop_table(new_relation->get_name());
}

vector<pair<DataValue, RelRange>> RelSorter::sort_return()
{
    vector<pair<DataValue, RelRange>> res;

    if (_relation->get_num_of_block() <= _mem_size) {
        RelScanner scanner(_relation, _base_idx, _mem_size);
        scanner.load_to_mem();
        scanner.sort(_attr_list);


        _relation->clear();
        scanner.add_mem_into(*_relation);

        if (_attr_list.size() == 1) {
            return _relation->get_sub_list_by_attr(_attr_list[0], 1);
        } else {
            return res;
        }
    }

    string new_table_name = "tmp " + _relation->get_name();
    TinyRelation* new_relation = HwMgr::ins()->create_relation(
        new_table_name, _relation->get_tiny_schema());


    RelScanner scanner(_relation, _base_idx, _mem_size);
    RelIter it = new_relation->end();
    vector<pair<RelIter, RelIter>> sub_list;
    while (!scanner.is_iter_end()) {
        scanner.load_to_mem();
        scanner.sort(_attr_list);
        scanner.add_mem_into(*new_relation);

        RelIter it_end = new_relation->end();
        sub_list.push_back(make_pair(it, it_end));
        it = it_end;
    }

    while (_mem_size < sub_list.size()) {
        new_relation = reduce_sub_list(new_relation, sub_list);
    }

    vector<RelScanner> scanner_list = get_scanner_list(new_relation, sub_list);

    _relation->clear();

    if (_attr_list.size() == 1) {
        string search_name = _relation->get_attr_search_name(_attr_list[0]);

        RelIter iter_end = _relation->begin();

        RelWriter writer(_relation);
        TinyTuple tuple = get_max(scanner_list);
        DataValue old_value = tuple.get_data_value(search_name);;
        while (!tuple.is_null()) {

            DataValue value = tuple.get_data_value(search_name);
            if (value != old_value) {
                writer.flush();
                RelIter iter = _relation->end();
                res.push_back( make_pair( old_value, RelRange(iter_end, iter)) );
                old_value = value;
                iter_end = iter;
            }

            writer.push_back(tuple);
            tuple = get_max(scanner_list);
        }
        writer.flush();
        res.push_back( make_pair( old_value, RelRange(iter_end, _relation->end())) );
    } else {
        RelWriter writer(_relation);
        TinyTuple tmp = get_max(scanner_list);
        while (!tmp.is_null()) {
            writer.push_back(tmp);
            tmp = get_max(scanner_list);
        }
        writer.flush();
    }

    HwMgr::ins()->drop_table(new_relation->get_name());
    return res;
}

TinyRelation* RelSorter::reduce_sub_list(TinyRelation* r, SubListType& sub_list)
{
    string new_table_name = "tmp " + r->get_name();
    TinyRelation* new_relation = HwMgr::ins()->create_relation(
        new_table_name, r->get_tiny_schema());

    vector<pair<RelIter, RelIter>> new_sub_list;

    vector<SubListType> sub_list_split = tiny::vector_split(sub_list, _mem_size);

    RelIter it = new_relation->end();
    for (const auto& tmp_sub_list : sub_list_split) {
        vector<RelScanner> scanner_list = get_scanner_list(r, tmp_sub_list);

        RelWriter writer(new_relation);
        TinyTuple tmp = get_max(scanner_list);
        while (!tmp.is_null()) {
            writer.push_back(tmp);
            tmp = get_max(scanner_list);
        }
        writer.flush();

        RelIter it_end = new_relation->end();
        new_sub_list.push_back(make_pair(it, it_end));
        it = it_end;
    }

    HwMgr::ins()->drop_table(r->get_name());
    sub_list = new_sub_list;
    return new_relation;
}

TinyTuple RelSorter::get_max(vector<RelScanner>& scanner_list)
{
    assert(scanner_list.size() <= _mem_size);

    vector<string> search_name_list = _relation->get_attr_search_name_list(_attr_list);
    TinyTuple res = _relation->create_null_tuple();
    RelScanner* min_scanner = NULL;
    for (auto& scanner : scanner_list) {
        TinyTuple tmp = scanner.peep_next();
        if (tmp.is_null())
            continue;

        if (res.is_null()) {
            res = tmp;
            min_scanner = &scanner;
        } else if (tmp.is_less_than_by_attr(res, search_name_list)) {
            res = tmp;
            min_scanner = &scanner;
        }
    }

    if (min_scanner != NULL) {
        min_scanner->get_next();
    }

    return res;
}

vector<RelScanner> RelSorter::get_scanner_list(TinyRelation* r, const SubListType& sub_list) const
{
    assert(sub_list.size() <= _mem_size);

    vector<RelScanner> res;
    for (size_t i = 0; i < sub_list.size(); ++i) {
        size_t mem_idx = _base_idx + i;
        const RelIter& it = sub_list[i].first;
        const RelIter& it_end = sub_list[i].second;

        res.push_back( RelScanner(r, mem_idx, 1) );
        res.back().set_begin(it);
        res.back().set_end(it_end);

    }

    return res;
}

//------------------------------------------------------------------------------
//   TinyRelation
//------------------------------------------------------------------------------

TinyRelation::TinyRelation(Relation* r)
: _relation(r)
, _pipe_queue(NULL)
, _with_prefix(false)
{
    assert(_relation != NULL);
}

TinyRelation::~TinyRelation()
{
    delete_not_null(_pipe_queue);
}

void TinyRelation::set_pipe_queue()
{
    _pipe_queue = new vector<TinyTuple>();
}

void TinyRelation::push_back(const TinyTuple& tuple)
{
    if (_pipe_queue != NULL) {
        _pipe_queue->push_back(tuple);
        return;
    }

    size_t mem_index = 0;
    TinyBlock block = HwMgr::ins()->get_mem_block(mem_index);
    if (next_is_new_block()) {
        block.clear();
        block.push_back(tuple);
        _relation->setBlock(_relation->getNumOfBlocks(), mem_index);
    } else {
        _relation->getBlock(_relation->getNumOfBlocks()-1, mem_index);
        block.push_back(tuple);
        _relation->setBlock(_relation->getNumOfBlocks()-1, mem_index);
    }
}

void TinyRelation::push_back_block(const vector<TinyTuple>& tuples)
{
    assert(tuples.size() <= get_last_block_capacity());

    if (_pipe_queue != NULL) {
        add_into(*_pipe_queue, tuples);
        return;
    }

    size_t mem_index = 0;
    TinyBlock block = HwMgr::ins()->get_mem_block(mem_index);
    if (next_is_new_block()) {
        block.clear();
        block.push_back(tuples);
        _relation->setBlock(_relation->getNumOfBlocks(), mem_index);
    } else {
        _relation->getBlock(_relation->getNumOfBlocks()-1, mem_index);
        block.push_back(tuples);
        _relation->setBlock(_relation->getNumOfBlocks()-1, mem_index);
    }
}

void TinyRelation::add_space(size_t block_idx, size_t tuple_idx)
{
    size_t space_idx = block_idx * tuple_per_block() + tuple_idx; 
    add_space(space_idx);
}

void TinyRelation::add_space(size_t space_idx)
{
    _space.push_back(space_idx);
    for (size_t i = _space.size() - 1; i > 0; --i) {
        assert(_space[i] != _space[i-1]);
        if (_space[i] < _space[i-1]) {
            swap(_space[i], _space[i-1]);
        }
    }
}

void TinyRelation::clear()
{
    _space.clear();
    reduce_blocks_to(0);
}

void TinyRelation::refresh_block_num()
{
    size_t total_size = size() + _space.size();
    for (size_t i = _space.size(); i-- > 0;) {
        if (_space[i] == total_size - 1) {
            total_size--;
            _space.pop_back();
        } else {
            break;
        }
    }

    size_t new_block_size = total_size / tuple_per_block();

    if (total_size % tuple_per_block() != 0)
        new_block_size++;

    reduce_blocks_to(new_block_size);
}

bool TinyRelation::load_block_to_mem(size_t block_index, size_t mem_index) const
{
    return _relation->getBlock(block_index, mem_index);
}

bool TinyRelation::save_block_to_disk(size_t block_index, size_t mem_index) const
{
    return _relation->setBlock(block_index, mem_index);
}

void TinyRelation::reduce_blocks_to(size_t block_num) const
{
    if (block_num < get_num_of_block()) {
        _relation->deleteBlocks(block_num);
    }
}

TinyTuple TinyRelation::create_tuple(bool is_null) const
{
    TinyTuple t = _relation->createTuple();
    t.init();
    if (is_null) {
        t.set_null();
    }
    return t;
}

string TinyRelation::get_name() const
{
    return _relation->getRelationName();
}

string TinyRelation::get_base_name() const
{
    vector<string> toks = tiny::tokenize(get_name(), " ");
    
    return toks.back();
}

string TinyRelation::get_attr_search_name(const ColumnName& column_name) const
{
    string res = column_name;

    string table = column_name.get_table();
    if (!table.empty() && !is_with_prefix() && table == get_base_name()) {
        res = column_name.get_column();
    }

    return res;
}

vector<string> TinyRelation::get_attr_search_name_list(const vector<string>& attr_list) const
{
    vector<string> res;
    for (const string& attr : attr_list) {
        string search_name = get_attr_search_name(attr);
        res.push_back(search_name);
    }
    return res;
}

TinySchema TinyRelation::get_tiny_schema() const
{
    return TinySchema(_relation->getSchema());
}

size_t TinyRelation::size() const
{
    return (size_t)_relation->getNumOfTuples();
}

size_t TinyRelation::size_by_block() const
{
    size_t res = size() / tuple_per_block();
    return res;
}

bool TinyRelation::empty() const
{
    return size() == 0;
}

size_t TinyRelation::get_num_of_attribute() const
{
    return get_tiny_schema().size();
}

vector<DataType> TinyRelation::get_type_list() const
{
    return get_tiny_schema().get_type_list();
}

vector<string> TinyRelation::get_attr_list() const
{
    return get_tiny_schema().get_attr_list();
}

vector<pair<string, DataType>> TinyRelation::get_attr_type_list() const
{
    return get_tiny_schema().get_attr_type_list();
}

vector<pair<string, DataType>> TinyRelation::get_attr_type_list_with_name() const
{
    if (is_with_prefix()) {
        return get_attr_type_list();
    }

    vector<pair<string, DataType>> attr_list = get_attr_type_list();
    vector<pair<string, DataType>> res;
    string relation_base_name = get_base_name(); // TODO
    for (const auto& name_type : attr_list) {
        const string& name = name_type.first;
        res.push_back( make_pair(relation_base_name + "." + name, name_type.second) );
    }
    return res;
}

size_t TinyRelation::get_num_of_block() const
{
    return (size_t)_relation->getNumOfBlocks();
}

size_t TinyRelation::tuple_per_block() const
{
    return get_tiny_schema().tuple_per_block();
}

size_t TinyRelation::get_block_idx_by_pos(size_t pos) const
{
    return pos / tuple_per_block();
}

size_t TinyRelation::get_tuple_idx_by_pos(size_t pos) const
{
    return pos % tuple_per_block();
}

pair<size_t, size_t> TinyRelation::get_idx_by_pos(size_t pos) const
{
    pair<size_t, size_t> tmp;
    tmp.first = pos / tuple_per_block();
    tmp.second = pos % tuple_per_block();
    return tmp;
}

const vector<TinyTuple>& TinyRelation::get_pipe_queue() const
{
    assert(_pipe_queue != NULL);

    return *_pipe_queue;
}

bool TinyRelation::is_null(size_t pos) const
{
    return is_contain(_space, pos);
}

bool TinyRelation::is_attr_exist(const string& name) const
{
    string search_name = get_attr_search_name(name);
    bool res = get_tiny_schema().is_field_name_exist(search_name);
    return res;
}

TinyRelation::iterator TinyRelation::begin() const
{
    return iterator(this);
}

TinyRelation::iterator TinyRelation::end() const
{
    size_t end_pos = get_total_pos();
    return iterator(this, end_pos);
}

size_t TinyRelation::get_total_pos() const
{
    return size() + _space.size();
}

bool TinyRelation::next_is_new_block() const
{
    return (size() + _space.size()) % tuple_per_block() == 0; 
}

size_t TinyRelation::get_last_block_capacity() const
{
    return tuple_per_block() - ((size() + _space.size()) % tuple_per_block()); 
}

vector<pair<DataValue, RelRange>> TinyRelation::get_sub_list_by_attr(
    const string& attr, size_t mem_idx) const
{
    assert(!attr.empty());
    assert(is_attr_exist(attr));

    string search_name = get_attr_search_name(attr);

    vector<pair<DataValue, RelRange>> res; 

    RelIter iter = begin();
    RelIter iter_end = iter;
    if (iter == end()) {
        return res;
    }

    size_t block_idx = iter.get_block_idx();
    TinyTuple tuple = iter.load_to_mem(mem_idx);
    DataValue old_value = tuple.get_data_value(search_name);;
    while (iter != end()) {
        if (block_idx == iter.get_block_idx()) {
            tuple = iter.get_from_mem(mem_idx);
        } else {
            tuple = iter.load_to_mem(mem_idx);
            block_idx = iter.get_block_idx();
        }
        DataValue value = tuple.get_data_value(search_name);

        if (value != old_value) {
            res.push_back( make_pair( old_value, RelRange(iter_end, iter)) );
            old_value = value;
            iter_end = iter;
        }
        ++iter;
    }

    res.push_back( make_pair( old_value, RelRange(iter_end, iter)) );

    return res;
}

void TinyRelation::print_table() const
{
    DrawTable table(get_num_of_attribute(), DrawTable::MYSQL_TABLE);
    vector<DataType> type_list = get_type_list();
    for (size_t i = 0; i < type_list.size(); ++i) {
        if (type_list[i] == TINY_INT) {
            table.set_align_right(i);
        }
    }

    table.set_header(get_attr_list());

    if (_pipe_queue != NULL) {
        for (const auto& tuple : *_pipe_queue) {
            table.add_row(tuple.get_str_list());
        }
    } else {
        RelScanner scanner(this, 1, 1);
        while(!scanner.is_end()) {
            TinyTuple tuple = scanner.get_next();
            assert(!tuple.is_null());

            table.add_row(tuple.get_str_list());
        }
    }

    if (table.size() == 0) {
        cout << "Empty set";
    } else {
        table.draw();
        cout << table.size() << " ";
        cout << ((table.size() == 1) ? "row" : "rows");
        cout << " in set";
    }

    cout << " (" << HwMgr::ins()->get_elapse_io() << " disk I/O, ";
    cout << HwMgr::ins()->get_elapse_time() << " ms)"<< endl;
}

void TinyRelation::dump() const
{
    DrawTable table(2, DrawTable::MYSQL_TABLE);
    table.add_row("name", get_name());
    table.add_row("size", tiny_dump_str(size()));
    table.add_row("# of blocks", tiny_dump_str(get_num_of_block()));
    table.add_row("tuple per block", tiny_dump_str(tuple_per_block()));
    table.add_row("_with_prefix", tiny_dump_str(_with_prefix));
    cout << "schema: " << get_tiny_schema().dump_str() << endl;
    cout << "_space" << tiny::dump_str(_space) << endl;
    table.draw();

    if (is_pipe_queue()) { 
        cout << "_pipe_queue" << endl;
        print_table();
    } else {
        dump_tuples();
    }
}

void TinyRelation::dump_tuples() const
{
    DrawTable table(get_num_of_attribute() + 3, DrawTable::MYSQL_TABLE);
    table.set_align_right(0);
    table.set_align_right(1);
    vector<DataType> type_list = get_type_list();
    for (size_t i = 0; i < type_list.size(); ++i) {
        if (type_list[i] == TINY_INT) {
            table.set_align_right(i+3);
        }
    }

    vector<string> header = {"i", "j", "d"};
    add_into(header, get_attr_list());
    table.set_header(header);

    size_t mem_index = 0;
    size_t num_of_block = get_num_of_block();
    for (size_t i = 0; i < num_of_block; ++i) {
        _relation->getBlock(i, mem_index);
        TinyBlock tiny_block = HwMgr::ins()->get_mem_block(mem_index);
        vector<TinyTuple> tuples = tiny_block.get_tiny_tuples();
        for (size_t j = 0; j < tuples.size(); ++j) {
            const TinyTuple& tuple = tuples[j];

            vector<string> row = {tiny::dump_str(i), tiny::dump_str(j)};
            if (tuple.is_null()) {
                row.push_back("D");
            } else {
                row.push_back("");
                add_into(row, tuple.get_str_list());
            }
            table.add_row(row);
        }
    }

    table.draw();
}

string TinyRelation::dump_str() const
{
    string tmp = get_name();
    tmp += " ";
    tmp += get_tiny_schema().dump_str();
    return tmp;
}
