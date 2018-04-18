#ifndef CREPORT_WRITER_JSON_H
#define CREPORT_WRITER_JSON_H

#include "CTString.h"

namespace CReportWriter {

  /**
   * Writes all current report messages to a file in a JSON format.
   *
   * The format of the JSON report equals:
   *
   * {
   *     "error": [
   *         "Error message 1."
   *         "Error message 2."
   *     ],
   *     "info": [
   *         "Info message 1."
   *         "Info message 2."
   *     ],
   *     "warning": [
   *         "Warning message 1."
   *         "Warning message 2."
   *     ]
   *
   * @param reportFilename The filename (including path) of the resulting file.
   * @returns True if it was possible to write the report to a file.
   */
  bool writeJSONReportToFile(const CT::string reportFilename);
};

#endif //CREPORT_WRITER_JSON_H
