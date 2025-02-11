from typing import Union


class QueryEngineBaseException(Exception):
    """Base class to exceptions in this module."""

    def __init__(self, message: str, details: str = ""):
        self.message = message
        self.details = details

        super().__init__(self.message, self.details)


class QueryParametersException(QueryEngineBaseException):
    """Exception raised for errors in the parameters of a query."""


class UnexpectedQueryFormat(QueryEngineBaseException):
    """Exception raised for errors in the format of a query."""


class InvalidAssignment(QueryEngineBaseException):
    """Exception raised for invalid assignments."""


class _ConnectionError(QueryEngineBaseException):
    """Exception raised for connection errors."""


class RetryConnectionError(_ConnectionError):
    """Exception raised for retry connection errors."""


class FunctionsConnectionError(_ConnectionError):
    """Exception raised for functions connection errors."""


class _TimeoutError(QueryEngineBaseException):
    """Exception raised for timeout errors."""


class FunctionsTimeoutError(_TimeoutError):
    """Exception raised for functions timeout errors."""


class HTTPError(QueryEngineBaseException):
    """Exception raised for HTTP errors."""

    def __init__(self, message: str, details: str = "", status_code: Union[int, None] = None):
        super().__init__(message, details)
        self.status_code = status_code


class RequestError(QueryEngineBaseException):
    """Exception raised for request errors."""


class RetryException(QueryEngineBaseException):
    """Exception raised for retry errors."""


class InvalidDASParameters(QueryEngineBaseException):
    """Exception raised for invalid DAS parameters."""


class InvalidQueryEngine(QueryEngineBaseException):
    """Exception raised for invalid query engines."""


class GetTraversalCursorException(QueryEngineBaseException):
    """Exception raised for errors in getting traversal cursor."""
