#!/usr/bin/python
import os, sys
os.environ['DJANGO_SETTINGS_MODULE'] = "levels.settings"
import django

from django.core.management import execute_manager
import levels.settings # Assumed to be in the same directory.

if __name__ == "__main__":
    execute_manager(levels.settings) 

