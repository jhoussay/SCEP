![SCEP logo](code/SCEP/resources/SCEP/images/SCEP-full.png "SCEP")

# SCEP

## Description

SCEP is a multi tab file explorer for Windows.

Each tab embeds a native file explorer window running in an external explorer.exe process.

SCEP is released under MIT license.

## Why

TODO

## Versions and Roadmap

### Minimum Viable Product

- [x] Embed an a file explorer window in Qt application
- [x] Embed multiple file explorer windows in QTabWidget

### V1.0

- [x] Address bar
- [x] Back and forward buttons
- [x] Application icon on exe file
- [x] Application menu
- [x] About window
- [ ] Tab context menu : close tab, duplicate tab, copy full path to clipboard, close other tabs
- [x] Close tab with middle click
- [ ] Installation program
- [ ] Handle "&" in tab text
- [ ] Doxygen comments and html generation
- [x] "Translate" pathes as explorer.exe does.
- [ ] Single instance
- [ ] Reopen closed tabs
- [x] Open new tab with middle click on a folder
- [x] Open new tab with a dedicated option in the context menu displayed for folders
- [ ] Detect media added or removed
- [ ] Handle shortcuts in both windows (Qt and win32) : CTRL + T, CTRL+W, CTRL+L...
- [x] Breadcrumb bar : highlight current folder (with bold font) in menus
- [ ] Enhance light palette to match the windows palette
- [ ] Restore window geometry at start
- [ ] Settings : show hidden files, single instance, theme color (dark, light, auto -> will need restart to take effect)
- [x] QCompleter : propose drives and known folders for non absolutes pathes
- [x] Handle known folders icons in address bar and tabs
- [ ] Buffering icon remains on navigation failed
- [x] Full support for translated paths
- [x] Double error message on navigation failure
- [x] Replace "/" by "\\" when pasting on address bar
- [x] Intermitent missing icon bug for virtual folders
- [ ] History menu on backward / forward buttons

### V2.0

- [ ] Search !
- [ ] Help window
- [ ] I18N
- [ ] Jump list
- [ ] Windows + Shift + E to set focus to running instance and open a new tab
  (requires a running instance !)
- [ ] Replace QTabWidget by Advanced Docking System : https://github.com/githubuser0xFFFF
- [ ] Ability to open a new instance
- [ ] Reopen closed instances

## Building from sources

TODO





# Assets
* [Right Arrow](https://www.iconfinder.com/icons/211607/right_arrow_icon), MIT
* [Left Arrow](https://www.iconfinder.com/icons/211689/left_arrow_icon), MIT