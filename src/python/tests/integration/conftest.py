import pytest

from tests.integration.helpers import remote_down, remote_up


def pytest_addoption(parser):
    parser.addoption(
        "--build", action="store_true", default=False, help="Rebuilds container's test image."
    )
    parser.addoption(
        "--no-destroy",
        action="store_true",
        default=False,
        help="Don't destroy container when test ends (faster test reruns).",
    )


@pytest.fixture(scope="session")
def environment_manager(request):
    build = request.config.getoption("--build")
    no_destroy = request.config.getoption("--no-destroy")
    remote_up(build)
    yield
    remote_down(no_destroy)
