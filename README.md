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

 - [x] Embed an explorer.exe window in Qt application
 - [x] Embed multiple explorer.exe windows in QTabWidget

**Known limitations**
 - Explorer windows blink when the user moves the SCEP window.
 - Opening a new tab is not very smooth and clean with multiple blinkings
 - [Rare] Opening a new tab results in an empty tab, SCEP does not manage to correctly "grab" the new launched explorer window.

### V1.0
 - [x] Application icon on exe file
 - [ ] Application menu
 - [x] About window
 - [ ] Tab context menu : close tab, duplicate tab, copy full path to clipboard
 - [ ] Installation program
 - [ ] Handle "&" in current folder
 - [ ] Handle user closing explorer in the tab

### V1.1

 - [ ] Doxygen comments and html generation
 - [ ] "Translate" paths as explorer.exe does.
Example : "C:\Users" -> "C:\Utilisateurs" in french
 - [ ] Single instance
 - [ ] Reopen closed tabs
 - [ ] Jump list
 - [ ] Windows + Shift + E to set focus to running instance and open a new tab
   (requires a running instance !)
 - [ ] Help window

### V1.2

 - [ ] Ability to open a new instance
 - [ ] Reopen closed instances
 - [ ] Close other tabs

### V2.0

- [ ] Replace QTabWidget by Advanced Docking System : https://github.com/githubuser0xFFFF


### ...

- [ ] I18N
- [ ] Handle explorer crash

## Building from sources

TODO

