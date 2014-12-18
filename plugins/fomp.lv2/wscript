#!/usr/bin/env python
import os
import shutil
import waflib.extras.autowaf as autowaf

# Version of this package (even if built as a child)
FOMP_VERSION = '1.0.0'

# Mandatory waf variables
APPNAME = 'fomp'        # Package name for waf dist
VERSION = FOMP_VERSION  # Package version for waf dist
top     = '.'           # Source directory
out     = 'build'       # Build directory

def options(opt):
    opt.load('compiler_cxx')
    autowaf.set_options(opt)

def configure(conf):
    conf.load('compiler_cxx')
    autowaf.configure(conf)
    autowaf.set_c99_mode(conf)
    autowaf.display_header('Fomp.LV2 Configuration')

    autowaf.check_pkg(conf, 'lv2', atleast_version='1.0.0', uselib_store='LV2')

    # Set env.pluginlib_PATTERN
    pat = conf.env.cxxshlib_PATTERN
    if pat[0:3] == 'lib':
        pat = pat[3:]
    conf.env.pluginlib_PATTERN = pat
    conf.env.pluginlib_EXT = pat[pat.rfind('.'):]

    autowaf.display_msg(conf, 'LV2 bundle directory', conf.env.LV2DIR)
    print('')

def build_plugin(bld, lang, bundle, name, source, defines=None):
    # Build plugin library
    penv = bld.env.derive()
    penv.cxxshlib_PATTERN = bld.env.pluginlib_PATTERN
    obj = bld(features     = '%s %sshlib' % (lang,lang),
              env          = penv,
              source       = source,
              includes     = ['.', 'src/include'],
              name         = name,
              target       = os.path.join(bundle, name),
              uselib       = ['LV2'],
              install_path = '${LV2DIR}/' + bundle)
    if defines != None:
        obj.defines = defines

def build(bld):
    # Copy data files to build bundle (build/fomp.lv2)
    def do_copy(task):
        src = task.inputs[0].abspath()
        tgt = task.outputs[0].abspath()
        return shutil.copy(src, tgt)

    for i in bld.path.ant_glob('fomp.lv2/*.ttl'):
        bld(features     = 'subst',
            is_copy      = True,
            source       = i,
            target       = bld.path.get_bld().make_node('fomp.lv2/%s' % i),
            install_path = '${LV2DIR}/fomp.lv2')

    bld(features     = 'subst',
        source       = 'fomp.lv2/manifest.ttl.in',
        target       = bld.path.get_bld().make_node('fomp.lv2/manifest.ttl'),
        LIB_EXT      = bld.env.pluginlib_EXT,
        install_path = '${LV2DIR}/fomp.lv2')

    plugins = ['autowah',
               'blvco',
               'cs_chorus',
               'cs_phaser',
               'filters',
               'mvchpf24',
               'mvclpf24']
    for i in plugins:
        build_plugin(bld, 'cxx', 'fomp.lv2', i,
                     ['src/%s.cc' % i,
                      'src/%s_lv2.cc' % i])
    build_plugin(bld, 'cxx', 'fomp.lv2', 'reverbs',
                 ['src/reverbs.cc',
                  'src/pareq.cc',
                  'src/zreverb.cc',
                  'src/reverbs_lv2.cc'])
