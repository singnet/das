"""
This module loads configuration settings from a .env file using the dotenv library.

It reads the key-value pairs from the .env file and stores them in a dictionary named `config`.
These settings can be used throughout the application to configure various parameters.
"""

from dotenv import dotenv_values

config = dotenv_values(".env")
