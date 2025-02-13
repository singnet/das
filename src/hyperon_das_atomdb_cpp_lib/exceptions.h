
/**
 * @file exceptions.h
 * @brief Defines custom exception classes for the Atom Database (atomdb) library.
 *
 * This header file contains the definitions of various exception classes used in the Atom
 * Database (atomdb) library. These exceptions are designed to handle specific error conditions
 * that may arise during the operation of the database. Each exception class inherits from
 * AtomDbBaseException, which in turn inherits from the standard std::exception class.
 *
 * The AtomDbBaseException class provides a mechanism to store and retrieve detailed error
 * messages. It uses a static buffer (`what_buffer`) to ensure that the exception message remains
 * valid and accessible even after the original C-string has been invalidated. This is
 * particularly useful when interfacing with libraries like `nanobind` that may lazily access
 * std::exception::what().
 *
 * The following custom exception classes are defined:
 * - AtomDoesNotExist: Thrown when an atom does not exist in the database.
 * - AddNodeException: Thrown when adding a node to the database fails.
 * - AddLinkException: Thrown when adding a link to the database fails.
 * - InvalidOperationException: Thrown when an invalid operation is performed on the database.
 * - RetryException: Raised for retryable errors.
 * - InvalidAtomDB: Raised for invalid Atom DB operations.
 *
 * Each of these exception classes inherits from AtomDbBaseException and can be used to provide
 * detailed error messages specific to the context in which the error occurred.
 */
#pragma once

#include <exception>
#include <string>

using namespace std;

namespace atomdb {

#define BUFFER_SIZE 4096

/**
 * @brief Buffer to store the exception message.
 * nanobind makes a lazy access to std::exception::what(), which can result in the
 * underlying C-string being lost or invalidated. To work around this issue, we
 * create a static buffer (what_buffer) to store the exception message. This ensures
 * that the message remains valid and accessible even after the original C-string
 * has been invalidated.
 */
static char what_buffer[BUFFER_SIZE];

class AtomDbBaseException : public exception {
   public:
    AtomDbBaseException(const string& message, const string& details)
        : message(message), details(details) {}

    virtual const char* what() const noexcept override {
        sprintf(what_buffer, "%s", (this->message + ": " + this->details).c_str());
        return what_buffer;
    }

   protected:
    string message;
    string details;
};

/**
 * @brief Exception thrown when an atom does not exist in the database.
 */
class AtomDoesNotExist : public AtomDbBaseException {
   public:
    using AtomDbBaseException::AtomDbBaseException;
};

/**
 * @brief Exception thrown when adding a node to the database fails.
 */
class AddNodeException : public AtomDbBaseException {
    using AtomDbBaseException::AtomDbBaseException;
};

/**
 * @brief Exception thrown when adding a link to the database fails.
 */
class AddLinkException : public AtomDbBaseException {
    using AtomDbBaseException::AtomDbBaseException;
};

/**
 * @brief Exception thrown when an invalid operation is performed on the database.
 */
class InvalidOperationException : public AtomDbBaseException {
    using AtomDbBaseException::AtomDbBaseException;
};

/**
 * @brief Exception raised for retryable errors.
 */
class RetryException : public AtomDbBaseException {
    using AtomDbBaseException::AtomDbBaseException;
};

/**
 * @brief Exception raised for invalid Atom DB operations.
 */
class InvalidAtomDB : public AtomDbBaseException {
    using AtomDbBaseException::AtomDbBaseException;
};

}  // namespace atomdb
