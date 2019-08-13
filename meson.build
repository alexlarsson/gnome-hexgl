project('gnome-hexgl', 'c',
        version: '0.0.1',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          'c_std=gnu99',
        ],
        meson_version : '>= 0.48.0')

gthree = dependency('gthree-1.0')
gsound = dependency('gsound')

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))

cc = meson.get_compiler('c')
libm = cc.find_library('m', required : false)

sources = ['src/hexgl.c', 'src/utils.c', 'src/analysismap.c', 'src/camerachase.c',
        'src/shipcontrols.c', 'src/shipeffects.c', 'src/shaders.c',
        'src/particles.c', 'src/hud.c', 'src/gameplay.c', 'src/sounds.c']

add_project_arguments(['-DDATADIR="@0@"'.format(datadir)], language: 'c')

install_subdir('models', install_dir: datadir, exclude_files: ['ships/feisar/feisar.blend','tracks/cityscape/cityscape.blend'])
install_subdir('textures', install_dir: datadir)

executable('gnome-hexgl', sources, dependencies: [gthree, gsound, libm], install: true)