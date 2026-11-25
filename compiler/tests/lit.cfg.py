# Lit configuration for BasicForge tests

import lit.formats
import os
import sys

config.name = "BasicForge"
config.test_format = lit.formats.ShTest(execute_external=True)

config.suffixes = ['.test']
config.test_source_root = os.path.dirname(__file__)
config.excludes = ['lit.cfg.py', 'README.md', 'lit.site.cfg.py']

# Substitutions - use CMake build directory
bfc_exe = os.environ.get('BFC_EXE', os.path.join(os.path.dirname(__file__), '..', 'build', 'Release', 'bfc.exe'))
config.substitutions.append(('%bfc', bfc_exe))
config.substitutions.append(('%t', 'test_output'))

# Environment
config.environment['PATH'] = os.environ['PATH']
config.environment['SYSTEMROOT'] = os.environ.get('SYSTEMROOT', 'C:\\Windows')

# Features
config.available_features.add('windows')
config.available_features.add('x86_64')

# Test timeout (seconds)
config.timeout = 120