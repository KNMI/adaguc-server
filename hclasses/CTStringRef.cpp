#include "CTString.h"
#include "CTStringRef.h"

namespace CT {

  CT::stringref::~stringref() {
    constdata = NULL;
    _length = 0;
    if (voldata != NULL) {
      delete[] voldata;
      voldata = NULL;
    }
  }

  CT::stringref::stringref() { init(); }

  CT::stringref::stringref(const char *data, size_t length) {
    init();
    assign(data, length);
  }

  CT::stringref::stringref(CT::stringref const &f) {
    init();
    _length = f._length;
    constdata = f.constdata;
  }

  CT::stringref &CT::stringref::operator=(stringref const &f) {
    if (this == &f) return *this;
    init();
    _length = f._length;
    constdata = f.constdata;
    return *this;
  }

  void CT::stringref::assign(const char *data, size_t length) {
    this->constdata = data;
    this->_length = length;
  }

  void CT::stringref::init() { voldata = NULL; }

  const char *CT::stringref::c_str() {
    delete[] voldata;
    voldata = new char[_length + 1];
    memcpy(voldata, constdata, _length);
    voldata[_length] = '\0';
    return voldata;
  }

  int CT::stringref::indexOf(char const *search) {
    if (_length == 0) return -1;
    if (search[0] == '\0') return -1;
    size_t i = 0;
    for (size_t j = 0; j < _length; j++) {
      if (constdata[j] == search[i]) {
        i++;
      } else {
        i = 0;
      }
      if (search[i] == '\0') return j - i + 1;
    }
    return -1;
  }

  CT::stringref CT::stringref::trim() {
    stringref r;
    r.assign(constdata, _length);
    int s = 0;
    for (size_t j = 0; j < _length; j++) {
      if (constdata[j] != ' ') {
        s = j;
        break;
      }
    }
    r.constdata = r.constdata + s;
    int e = _length - s;
    for (int j = _length - 1 - s; j >= 0; j--) {
      if (r.constdata[j] != ' ') {
        e = j;
        break;
      }
    }
    r._length = (e + 1);
    return r;
  }

  size_t CT::stringref::length() { return _length; }

  std::vector<CT::stringref> CT::stringref::splitAsStringReferences(const char *_value) {
    std::vector<CT::stringref> stringList;
    stringref str(this->constdata, this->_length);
    int a = this->indexOf(_value);
    size_t rel = 0;
    while (a != -1) {
      if (a >= 0) {
        str._length = a;
        stringList.push_back(str);
      }
      a++;
      rel += a;
      str.constdata += a;
      str._length = this->_length - rel;
      a = str.indexOf(_value);
    }
    stringList.push_back(str);
    return stringList;
  }

  /*    int32::int32(){
          value=0;
          init();
      }
      int32::int32(int _value){
          init();
          value=_value;
      }*/

} /* namespace CT */
