#include "CCDFAttribute.h"
namespace CDF {
  void Attribute::setName(const char *value) { name.copy(value); }

  Attribute::Attribute() {
    data = NULL;
    length = 0;
  }

  Attribute::Attribute(Attribute *att) {
    data = NULL;
    length = 0;

    name.copy(&att->name);
    setData(att);
  }

  Attribute::Attribute(const char *attrName, const char *attrString) {
    data = NULL;
    length = 0;
    name.copy(attrName);
    setData(CDF_CHAR, attrString, strlen(attrString));
  }

  Attribute::Attribute(const char *attrName, CDFType type, const void *dataToSet, size_t dataLength) {
    data = NULL;
    length = 0;
    name.copy(attrName);
    setData(type, dataToSet, dataLength);
  }

  Attribute::~Attribute() {
    if (data != NULL) {
      freeData(&data);
    };
    data = NULL;
  }

  CDFType Attribute::getType() { return type; }

  int Attribute::setData(Attribute *attribute) {
    this->setData(attribute->type, attribute->data, attribute->size());
    return 0;
  }
  int Attribute::setData(CDFType type, const void *dataToSet, size_t dataLength) {
    if (data != NULL) {
      freeData(&data);
    };
    data = NULL;
    length = dataLength;
    this->type = type;
    allocateData(type, &data, length);
    if (type == CDF_CHAR || type == CDF_UBYTE || type == CDF_BYTE) memcpy(data, dataToSet, length);
    if (type == CDF_SHORT || type == CDF_USHORT) memcpy(data, dataToSet, length * sizeof(short));
    if (type == CDF_INT || type == CDF_UINT) memcpy(data, dataToSet, length * sizeof(int));
    if (type == CDF_INT64 || type == CDF_UINT64) memcpy(data, dataToSet, length * sizeof(long));
    if (type == CDF_FLOAT) memcpy(data, dataToSet, length * sizeof(float));
    if (type == CDF_DOUBLE) {
      memcpy(data, dataToSet, length * sizeof(double));
    }
    return 0;
  }

  int Attribute::setData(const char *dataToSet) {
    if (data != NULL) {
      freeData(&data);
    };
    data = NULL;
    length = strlen(dataToSet);
    this->type = CDF_CHAR;
    allocateData(type, &data, length + 1);
    if (type == CDF_CHAR) {
      memcpy(data, dataToSet, length); // TODO support other data types as well
      ((char *)data)[length] = '\0';
    }
    return 0;
  }

  int Attribute::getDataAsString(CT::string *out) {
    out->copy("");
    if (type == CDF_CHAR) {
      out->copy((const char *)data, length);
      int a = strlen(out->c_str());
      out->setSize(a);
      return 0;
    }
    if (type == CDF_BYTE)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%d", ((char *)data)[n]);
      }
    if (type == CDF_UBYTE)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%u", ((unsigned char *)data)[n]);
      }

    if (type == CDF_INT)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%d", ((int *)data)[n]);
      }
    if (type == CDF_UINT)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%u", ((unsigned int *)data)[n]);
      }

    if (type == CDF_INT64)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%ld", ((long *)data)[n]);
      }
    if (type == CDF_UINT64)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%lu", ((unsigned long *)data)[n]);
      }

    if (type == CDF_SHORT)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%d", ((short *)data)[n]);
      }
    if (type == CDF_USHORT)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%u", ((unsigned short *)data)[n]);
      }

    if (type == CDF_FLOAT)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%f", ((float *)data)[n]);
      }
    if (type == CDF_DOUBLE)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%f", ((double *)data)[n]);
      }
    if (type == CDF_STRING)
      for (size_t n = 0; n < length; n++) {
        if (out->length() > 0) out->concat(" ");
        out->printconcat("%s", ((char **)data)[n]);
      }
    return 0;
  }

  CT::string Attribute::toString() {
    CT::string out = "";
    getDataAsString(&out);
    return out;
  }

  CT::string Attribute::getDataAsString() { return toString(); }

  size_t Attribute::size() { return length; }
} // namespace CDF