package = "luigi"
version = "local-1"
source = {
   url = "git://github.com/airstruck/luigi.git"
}
description = {
   summary = "Lovely User Interfaces for Game Inventors",
   detailed = "Löve-compatible GUI library",
   homepage = "https://love2d.org/wiki/LUIGI",
   license = "MIT"
}
dependencies = {
   "lua ~> 5.1"
}
build = {
   type = "builtin",
   modules = {
      ["luigi.mosaic"] = "luigi/mosaic.lua",
      ["luigi.event"] = "luigi/event.lua",
      ["luigi.widget.slider"] = "luigi/widget/slider.lua",
      ["luigi.widget.check"] = "luigi/widget/check.lua",
      ["luigi.widget.radio"] = "luigi/widget/radio.lua",
      ["luigi.widget.progress"] = "luigi/widget/progress.lua",
      ["luigi.widget.menu"] = "luigi/widget/menu.lua",
      ["luigi.widget.status"] = "luigi/widget/status.lua",
      ["luigi.widget.window"] = "luigi/widget/window.lua",
      ["luigi.widget.menu.item"] = "luigi/widget/menu/item.lua",
      ["luigi.widget.sash"] = "luigi/widget/sash.lua",
      ["luigi.widget.stepper"] = "luigi/widget/stepper.lua",
      ["luigi.widget.text"] = "luigi/widget/text.lua",
      ["luigi.widget.button"] = "luigi/widget/button.lua",
      ["luigi.utf8"] = "luigi/utf8.lua",
      ["luigi.multiline"] = "luigi/multiline.lua",
      ["luigi.hooker"] = "luigi/hooker.lua",
      ["luigi.shortcut"] = "luigi/shortcut.lua",
      ["luigi.attribute"] = "luigi/attribute.lua",
      ["luigi.backend.ffisdl.keyboard"] = "luigi/backend/ffisdl/keyboard.lua",
      ["luigi.backend.ffisdl.sdl"] = "luigi/backend/ffisdl/sdl.lua",
      ["luigi.backend.ffisdl.font"] = "luigi/backend/ffisdl/font.lua",
      ["luigi.backend.ffisdl.spritebatch"] = "luigi/backend/ffisdl/spritebatch.lua",
      ["luigi.backend.ffisdl.sdl2.defines"] = "luigi/backend/ffisdl/sdl2/defines.lua",
      ["luigi.backend.ffisdl.sdl2"] = "luigi/backend/ffisdl/sdl2/init.lua",
      ["luigi.backend.ffisdl.sdl2.cdefs"] = "luigi/backend/ffisdl/sdl2/cdefs.lua",
      ["luigi.backend.ffisdl.image"] = "luigi/backend/ffisdl/image.lua",
      ["luigi.backend.ffisdl.text"] = "luigi/backend/ffisdl/text.lua",
      ["luigi.backend.love.font"] = "luigi/backend/love/font.lua",
      ["luigi.backend.love.text"] = "luigi/backend/love/text.lua",
      ["luigi.backend.ffisdl"] = "luigi/backend/ffisdl.lua",
      ["luigi.backend.love"] = "luigi/backend/love.lua",
      ["luigi.layout"] = "luigi/layout.lua",
      ["luigi.theme.dark"] = "luigi/theme/dark.lua",
      ["luigi.theme.light-big"] = "luigi/theme/light-big.lua",
      ["luigi.theme.light"] = "luigi/theme/light.lua",
      ["luigi.painter"] = "luigi/painter.lua",
      ["luigi.base"] = "luigi/base.lua",
      ["luigi.input"] = "luigi/input.lua",
      ["luigi.backend"] = "luigi/backend.lua",
      ["luigi.widget"] = "luigi/widget.lua",
      ["luigi.engine.alpha"] = "luigi/engine/alpha.lua",
      ["luigi.style"] = "luigi/style.lua",
      ["luigi.launch"] = "luigi/launch.lua",
   },
   copy_directories = {"luigi/theme/light", "luigi/theme/dark"}
}
