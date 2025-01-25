from unittest.mock import Mock, patch

import pytest

from hyperon_das.decorators import retry
from hyperon_das.exceptions import RetryConnectionError

logger_mock = Mock()


@patch('hyperon_das.logger')
def test_retry_successful_connection(logger_mock):
    @retry(attempts=3, timeout_seconds=5)
    def successful_function(self, host, port):
        return 200, "Success"

    result = successful_function({}, 'localhost', 80)

    assert result == (200, 'Success')


@patch('hyperon_das.logger')
def test_retry_exception_raised(logger_mock):
    @retry(attempts=3, timeout_seconds=5)
    def exception_function():
        raise ValueError("Simulated exception")

    with pytest.raises(
        RetryConnectionError, match='An error occurs while connecting to the server'
    ):
        exception_function()
