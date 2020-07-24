project('gnome-hexgl', 'c',
        version: '0.2.0',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          'c_std=gnu99',
        ],
        meson_version : '>= 0.49.0')

gthree_dep = dependency('gthree-gtk3-1.0', version: '>= 0.9.0',
                        fallback: ['gthree', 'libgthree_dep'])
gsound_dep = dependency('gsound')

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))

icondir = join_paths(datadir, 'icons', 'hicolor', '256x256', 'apps')
install_data(
  'org.gnome.HexGL.png',
  install_dir: icondir
)

desktopdir = join_paths(datadir, 'applications')
install_data(
  'org.gnome.HexGL.desktop',
  install_dir: desktopdir
)

appdatadir = join_paths(datadir, 'metainfo')
install_data(
  'org.gnome.HexGL.appdata.xml',
  install_dir: appdatadir
)

cc = meson.get_compiler('c')
libm = cc.find_library('m', required : false)

sources = ['src/hexgl.c', 'src/utils.c', 'src/analysismap.c', 'src/camerachase.c',
        'src/shipcontrols.c', 'src/shipeffects.c', 'src/shaders.c',
        'src/particles.c', 'src/hud.c', 'src/gameplay.c', 'src/sounds.c']

add_project_arguments(['-DDATADIR="@0@"'.format(join_paths(datadir , 'gnome-hexgl'))], language: 'c')

install_subdir('models', install_dir: join_paths(datadir , 'gnome-hexgl'), exclude_files: ['ships/feisar/feisar.blend','tracks/cityscape/cityscape.blend'])
install_subdir('textures', install_dir: join_paths(datadir , 'gnome-hexgl'))
install_subdir('sounds', install_dir: join_paths(datadir , 'gnome-hexgl'))

executable('gnome-hexgl', sources, dependencies: [gthree_dep, gsound_dep, libm], install: true)

configure_file(input: 'wrapper.sh.in', output: 'wrapper.sh', configuration: { 'srcdir': meson.current_source_dir()} )
