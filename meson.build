project('gnome-hexgl', 'c',
        version: '0.0.1',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          'c_std=gnu99',
        ],
        meson_version : '>= 0.48.0')

gthree = dependency('gthree-1.0')

cc = meson.get_compiler('c')
libm = cc.find_library('m', required : false)

sources = ['src/hexgl.c', 'src/utils.c', 'src/analysismap.c', 'src/camerachase.c', 'src/shipcontrols.c', 'src/shipeffects.c', 'src/shaders.c', 'src/particles.c', 'src/hud.c']

executable('gnome-hexgl', sources, dependencies: [gthree, libm])
