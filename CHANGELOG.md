<a name="1.1"></a>
## 1.1 (2018-10-11)


#### Bug Fixes

*   fix: input text can be read-only mode. ([fe4e93f](https://github.com/linuxdeepin/deepin-editor/commit/7e690af0fbda19735e4790e40adfe381f382aa57))



<a name="1.0"></a>
## 1.0 (2018-10-10)


#### Bug Fixes

*   multi-screen new window position is incorrect. ([f34ef80c](https://github.com/linuxdeepin/deepin-editor/commit/f34ef80c0842d331fad95cba6dd58eada2a0e379))
*   cannot preview shortcuts in read-only mode ([507edc2e](https://github.com/linuxdeepin/deepin-editor/commit/507edc2ed09063100fd01379a527b552ab1e81b6))
*   optimize close tabs func. ([e1e573b0](https://github.com/linuxdeepin/deepin-editor/commit/e1e573b08921c06cf2cfef0f3fd3debf317f6736))
*   no translation files included ([a6231c42](https://github.com/linuxdeepin/deepin-editor/commit/a6231c4210ea99fa7b0b17c5dcf161242271b02b))
*   tab drag out only disconnect the window siganl. ([da27ede7](https://github.com/linuxdeepin/deepin-editor/commit/da27ede799af5f05ec34be11b69debad8badc158))
*   drag tab to anther window signal is incorrect. ([08c5b927](https://github.com/linuxdeepin/deepin-editor/commit/08c5b92764a92a6949fddd1036e12f4e1cd3599b))
*   no highlight current line ([a3391be5](https://github.com/linuxdeepin/deepin-editor/commit/a3391be5be423701c2abd7d4b1ac265aae34d33c))
*   use fixed byte detection encoding. ([a0dd1cb5](https://github.com/linuxdeepin/deepin-editor/commit/a0dd1cb51fc48ee03a7bc61b03b6c86f7a1b0181))
*   ensure that the file is written to disk ([a036a13b](https://github.com/linuxdeepin/deepin-editor/commit/a036a13b77a47b5175272037b22c07059b6100e8))
*   line number padding. ([ed30c88b](https://github.com/linuxdeepin/deepin-editor/commit/ed30c88b0cd8b8e2ed30772b0db0da16a50d216a))
*   read-only mode cannot drag text. ([a3e4398b](https://github.com/linuxdeepin/deepin-editor/commit/a3e4398b60628db9db2e03177fa5c83ea10a1082))
* **menu:**
  *  change the highlight order ([1e90cbcc](https://github.com/linuxdeepin/deepin-editor/commit/1e90cbcc9a52df537fcf0fdf2c1677bde4d436f9))
  *  set minimum width ([47a0b06d](https://github.com/linuxdeepin/deepin-editor/commit/47a0b06d76289e7a06d591b8b46a3301db3b48d1))

#### Features

*   add abbreviated name. ([3250f36c](https://github.com/linuxdeepin/deepin-editor/commit/3250f36c6bc2980599a58565008436e24628cd22))
*   close tabs prompted to save the file. ([e54bdc99](https://github.com/linuxdeepin/deepin-editor/commit/e54bdc99de05210a16bcc0b002999d2469e42b65))
*   optimize file loading speed. ([286db0fc](https://github.com/linuxdeepin/deepin-editor/commit/286db0fc8092dcd6098ed8dc27430e2339c3fb41))
*   use cmake ([a56c06fb](https://github.com/linuxdeepin/deepin-editor/commit/a56c06fb04bf8d48d0aa28cd49b407641a0b2470))
*   add highlight right option ([501f7795](https://github.com/linuxdeepin/deepin-editor/commit/501f77958ccc62050dc1c756b9272355e5191f61))
* **tabbar:**  add right menu. ([053ec629](https://github.com/linuxdeepin/deepin-editor/commit/053ec62953415ebc912712abd7dd4e69e10d8b69))



<a name="0.5"></a>
## 0.5 (2018-09-21)


#### Bug Fixes

*   don't blank the cursor when selecting text ([8cf929f9](https://github.com/linuxdeepin/deepin-editor/commit/8cf929f9854bc4cf7e82c10a6c1b2e8898847c5a))
*   no permissions to read file without create new tab. ([bc4039e2](https://github.com/linuxdeepin/deepin-editor/commit/bc4039e2ddd522a7dcd5dd1bca74436a3b18ba77))
*   open invalid prompt ([7c81857e](https://github.com/linuxdeepin/deepin-editor/commit/7c81857eee11e6cdc8addc21ca881f31520477c4))
*   kill backward & forward word. ([47a566bb](https://github.com/linuxdeepin/deepin-editor/commit/47a566bba4af04f2dde610649ea096acf3f30f71))
*   can't replace all empty ([9b83f153](https://github.com/linuxdeepin/deepin-editor/commit/9b83f153bc607460a747c64365cae0b8fe833526))
*   no history filepath saved when closing other tabs. ([adff4cf5](https://github.com/linuxdeepin/deepin-editor/commit/adff4cf5138b0863c00cf5197dbb318e4efc26dd))
*   sort shortcuts ([6487ccf5](https://github.com/linuxdeepin/deepin-editor/commit/6487ccf52214423e55f3d4d4d0a3599beaeed288))
*   open again will create a new window ([7ed3ce95](https://github.com/linuxdeepin/deepin-editor/commit/7ed3ce95db97599fee198c93ee184c37618a75d3))
*   toggle find & replace bar focus ([e8370db4](https://github.com/linuxdeepin/deepin-editor/commit/e8370db401649bd9f9669dbefc611b94e102790e))
*   cannot be replaced after undo ([6fcab391](https://github.com/linuxdeepin/deepin-editor/commit/6fcab391ec2da47a4356ca396ad748f4776a43f6))
*   shortcut viewer translation. ([0d408be2](https://github.com/linuxdeepin/deepin-editor/commit/0d408be276587b75cbf57598e40291c6f358069e))
*   window size. ([15a3c805](https://github.com/linuxdeepin/deepin-editor/commit/15a3c805f581282f39a7567187cbce9d101d09ad))
*   goto next & previous word. ([b3440462](https://github.com/linuxdeepin/deepin-editor/commit/b3440462922997593847dc583465424d6344bc5c))
*   add tab space minimum value ([35ef2b9b](https://github.com/linuxdeepin/deepin-editor/commit/35ef2b9bcb4732c5b4653172ca0815935bcc3bda))
*   can be infinitely replaced next ([e249dc39](https://github.com/linuxdeepin/deepin-editor/commit/e249dc39538ab53c3d479c306974b1b43987e369))
*   replace text is empty. ([d6872e1f](https://github.com/linuxdeepin/deepin-editor/commit/d6872e1fd200d6fb14f238dcf62d91765693c43b))
*   do not restore tab when restoring pos ([d86ec9f0](https://github.com/linuxdeepin/deepin-editor/commit/d86ec9f0eb6b0203608020e963f61d3c52976095))
*   save prompt error ([5992d3ae](https://github.com/linuxdeepin/deepin-editor/commit/5992d3ae028197f78832c9ebccd763444f72f518))
*   new tab does not scroll the latest index. ([1aee05af](https://github.com/linuxdeepin/deepin-editor/commit/1aee05af722f1313ca966722b6d6d484186bcc4e))
*   printer output format ([5c37338f](https://github.com/linuxdeepin/deepin-editor/commit/5c37338f01620a2735785d349e067f9637629bb1))
*   new blank document cannot be saved. ([b5902cfe](https://github.com/linuxdeepin/deepin-editor/commit/b5902cfe607265273211c7cc65b743359d843ca9))
*   don't display horizontal scrollbar. ([328e221b](https://github.com/linuxdeepin/deepin-editor/commit/328e221bb05ad94fa6bc9313975df3670c446659))
*   blank document close without prompting to save. ([48578887](https://github.com/linuxdeepin/deepin-editor/commit/4857888738bfd50c96695fdffa13281c5e15d028))
*   font debug prompt. ([98f6eb40](https://github.com/linuxdeepin/deepin-editor/commit/98f6eb406ec85b75d3a69a44850a0ad66455e1e8))
*   draft file default save directory. ([aac54afc](https://github.com/linuxdeepin/deepin-editor/commit/aac54afcb1e5616672ea047b4c38a96d0ced422e))
*   scroll line incorrect. ([e5f832ce](https://github.com/linuxdeepin/deepin-editor/commit/e5f832cec1c8cef3d16db839aaddb94f462389d8))
*   file not finish loadding cannot be saved. ([2e75eabc](https://github.com/linuxdeepin/deepin-editor/commit/2e75eabc5743e17434815f0e5f1f24b87ffecf1e))
*   does not support highlight do not reload. ([e5bf3942](https://github.com/linuxdeepin/deepin-editor/commit/e5bf39422744c0b8b5a289e0896e7f98b6843339))
*   response esc keymap. ([55b9611a](https://github.com/linuxdeepin/deepin-editor/commit/55b9611a1e966d8dce366508ad012d747cc6f151))
*   save the file priority. ([382c58a0](https://github.com/linuxdeepin/deepin-editor/commit/382c58a0910dab5287dff8c93ee95a4ffbf0ab18))
*   popup the theme panel slow. ([719f1c29](https://github.com/linuxdeepin/deepin-editor/commit/719f1c295a3676fcb702c32bab4abf4ef0b96367))
*   don't show the toast when saving. ([5c3eb6af](https://github.com/linuxdeepin/deepin-editor/commit/5c3eb6af46c1d820439f2999b894e8a6a8be6f49))
*   best encoding it has guessed ([cda92ee7](https://github.com/linuxdeepin/deepin-editor/commit/cda92ee71e03e7e15c127f93015a66ef3b834965))
*   first theme panel popuped up, correct item is not selected. ([0323074c](https://github.com/linuxdeepin/deepin-editor/commit/0323074c33d695d4964112c157f30315c5520616))
*   save root file garbled. ([720b7ec7](https://github.com/linuxdeepin/deepin-editor/commit/720b7ec73a419e0a19aebd4f7222dede7b502a41))
*   determine whether writable. ([dac3c01c](https://github.com/linuxdeepin/deepin-editor/commit/dac3c01c58408c7aa763b670d2e0799e1db8f95c))
*   right click selection color. ([28f91304](https://github.com/linuxdeepin/deepin-editor/commit/28f91304c0f81a1f7ccb6128b5258171ef3669a9))
*   save as correct encoding. ([3693b410](https://github.com/linuxdeepin/deepin-editor/commit/3693b410f37c3083d07c2bb09a8764e695e12360))
*   save set encoding. ([3355588e](https://github.com/linuxdeepin/deepin-editor/commit/3355588eece2c2a88b1a9fabb286a070948341e0))
*   best encoding it has guessed so far. ([52aa370a](https://github.com/linuxdeepin/deepin-editor/commit/52aa370a7c5571fb910c696ad1a8801a093a163e))
*   drag move event crash ([9db195cd](https://github.com/linuxdeepin/deepin-editor/commit/9db195cd4a9b37132c37e2c3e4f6f5706ae6c21d))
*   save root file logic. ([e17002ea](https://github.com/linuxdeepin/deepin-editor/commit/e17002ea20daa014d6f336a679b04e40e526ed29))
*   pro file installs script ([cf6e0b7a](https://github.com/linuxdeepin/deepin-editor/commit/cf6e0b7afdca248de86cb9995b6bb76b081b0cdb))
*   save root file crash ([e1a6b660](https://github.com/linuxdeepin/deepin-editor/commit/e1a6b66060dee283a0d306c3e0b836e213629adb))
*   prompt to save the draft document. ([3232017b](https://github.com/linuxdeepin/deepin-editor/commit/3232017ba94783b2460e80cefbc1bafb0fdf23db))
*   restore cursor ([0c8123c9](https://github.com/linuxdeepin/deepin-editor/commit/0c8123c94c5e41c18d4928b29153b5932f45804e))
*   switch theme to scroll to the next line. ([abde4fac](https://github.com/linuxdeepin/deepin-editor/commit/abde4fac67410a28b12c7c462c3ae6b490f01279))
*   some themes do not highlight. ([15fd8fbd](https://github.com/linuxdeepin/deepin-editor/commit/15fd8fbde04fb81fa13f3f0d46ae20efefc153ad))
*   remove loadding cursor ([c96e9642](https://github.com/linuxdeepin/deepin-editor/commit/c96e9642f5f4eb991463aa2c349cff4c39bd8c9a))
*   move document thread ([862499bf](https://github.com/linuxdeepin/deepin-editor/commit/862499bf997a8bef3999d21c29be33621b60b42a))
*   do not create new QTextDocument. ([c143fd34](https://github.com/linuxdeepin/deepin-editor/commit/c143fd34d33a3a73ba65b967ab526bb8d11593ea))
*   open blank files of last session may cause window flash ([480caa69](https://github.com/linuxdeepin/deepin-editor/commit/480caa6918c982cafb87086ff4a8479d1015c217))
*   open blank files of last session may cause window flash. ([08d04190](https://github.com/linuxdeepin/deepin-editor/commit/08d041906cac46a2589d2b552d2b708d59ba74bb))
*   clear blank files that have no content. ([956b9e06](https://github.com/linuxdeepin/deepin-editor/commit/956b9e066260983cccc74d1db6a7b9a14fbeaa40))
*   if close the tab, delete the blank file. ([67f0ab17](https://github.com/linuxdeepin/deepin-editor/commit/67f0ab179203531d2f6c7d44a4cdd063288e3374))
*   findbar press ESC is invalid ([d900a205](https://github.com/linuxdeepin/deepin-editor/commit/d900a205968aaa2284100ac618229368cbfb5d4e))
*   texteditor does not have focus after switching tabs ([c2dc270e](https://github.com/linuxdeepin/deepin-editor/commit/c2dc270e9a0570f5d5c868f001792fe6db198c12))
*   use build version ([30fb5769](https://github.com/linuxdeepin/deepin-editor/commit/30fb5769fc19f2216f296629fe880af435b20738))
*   tab drag leave not close is incorrect. ([1b49bcef](https://github.com/linuxdeepin/deepin-editor/commit/1b49bcef26a46a51567e238351125b2a97696c78))
*   connection repeat slot functions. ([1c2b4ade](https://github.com/linuxdeepin/deepin-editor/commit/1c2b4ade3ac668bf87b169523fd074268e19c29f))
*   drag a window does not close properly when ([7a9c8bdc](https://github.com/linuxdeepin/deepin-editor/commit/7a9c8bdc7bd96906328669bea8490fdb45f3ffaf))
*   cursor stops being redrawn when QPlainTextEdit::dropEvent() overrided... ([fea5a7e7](https://github.com/linuxdeepin/deepin-editor/commit/fea5a7e7898146dd3385af64db8b36b8e353a769))
*   drag and drop tab index is incorrect ([04916d14](https://github.com/linuxdeepin/deepin-editor/commit/04916d14e7bbea469790d00ed37d2038af70cba9))
*   set window minimum size. ([a1d3b21b](https://github.com/linuxdeepin/deepin-editor/commit/a1d3b21b0a9218120ddf293ca3ca443a0c9602fe))
*   close event ([eafc9523](https://github.com/linuxdeepin/deepin-editor/commit/eafc95236a7ad5c3e401be7c5af65d66b45bf934))
*   no need to adjust scrollbar margins if reach last line ([4c56f4f1](https://github.com/linuxdeepin/deepin-editor/commit/4c56f4f1dc6860b6b3a8a3cdc4613121f9b6b1f0))
*   not release memory when close tab. ([52f0ea3f](https://github.com/linuxdeepin/deepin-editor/commit/52f0ea3f2fc835fae7cb84ac5e26c273cec28019))
*   tabbar stylesheet ([4aeea8a0](https://github.com/linuxdeepin/deepin-editor/commit/4aeea8a04feea2980ddf959730e4ad4dbe1088e3))
*   all window switching theme. ([46818c99](https://github.com/linuxdeepin/deepin-editor/commit/46818c995444ee18bd71101df52906f90635edc2))
*   theme setting background invalid. ([5a856d34](https://github.com/linuxdeepin/deepin-editor/commit/5a856d34580785e6f0a49ddc71d8ea872427eadc))
*   remove english completer menu. ([54e30108](https://github.com/linuxdeepin/deepin-editor/commit/54e3010819214b76a41dcf38890fb2476dde4046))
*   copying to clipboard text is incorrect. ([28a73ada](https://github.com/linuxdeepin/deepin-editor/commit/28a73adaa098fc95743ae21f2ecac5bbbb4d79a2))
*   window flashing. ([c55c5e42](https://github.com/linuxdeepin/deepin-editor/commit/c55c5e42713cc26a4699d79c18ed31105cdd17e0))
*   do not generate BOM. ([fec6fa62](https://github.com/linuxdeepin/deepin-editor/commit/fec6fa6224d252a3b1df43c2ba10741b97ca34e8))
*   read-only mode moving cursor crashes. ([5042c39e](https://github.com/linuxdeepin/deepin-editor/commit/5042c39ef5a9ccaa2c71d3604b131a23b3e2a838))
*   save the correct encoding ([a9e92874](https://github.com/linuxdeepin/deepin-editor/commit/a9e92874e4e25e1ea99362fa42bb27ff1b1f5901))
*   crash when drag tab on high qt version ([1cecc879](https://github.com/linuxdeepin/deepin-editor/commit/1cecc879b238b1a2fd1e8092a1d79537a5dc6fa3))
* **SettingsDialog:**
  *  no update tab space number. ([d44b100b](https://github.com/linuxdeepin/deepin-editor/commit/d44b100b9eb9c4269a06fc916876de53898196af))
  *  font is empty after recovery. ([2683caea](https://github.com/linuxdeepin/deepin-editor/commit/2683caeac5611b43a1a9cfe0bfe7850ace6ac0e9))
* **config:**  desktop translation ([15b4f878](https://github.com/linuxdeepin/deepin-editor/commit/15b4f87857fd6c87e30f21f2fc6cdc10625aa5e9))
* **find:**  makes find match only complete words. ([c2de5c36](https://github.com/linuxdeepin/deepin-editor/commit/c2de5c364b84a8f6648c4e789e1c742475814014))
* **find_bar:**  text display is incomplete, increase height. ([f282ab24](https://github.com/linuxdeepin/deepin-editor/commit/f282ab249196a9ac19d1a224077aa2f9d8f5deac))
* **read_only_mode:**  empty text moving cursor crashes. ([f60be78e](https://github.com/linuxdeepin/deepin-editor/commit/f60be78e47e11680156bcc2bc6fb515c229e2955))
* **tabbar:**
  *  close tab without reminder. ([c846f6e9](https://github.com/linuxdeepin/deepin-editor/commit/c846f6e974c97af4d57e903f2677516ece4d0013))
  *  handleTabDroped() type conversion error. ([a1796f93](https://github.com/linuxdeepin/deepin-editor/commit/a1796f93adf45982ab70c53e0b0fb4ac370ce9fc))
  *  dnd background error ([8934d1e9](https://github.com/linuxdeepin/deepin-editor/commit/8934d1e965ffba5134c1c20291ab9d310edebe3a))
  *  did not call closeFile() after dragging out. ([33687534](https://github.com/linuxdeepin/deepin-editor/commit/336875340ed0f05c627e40f650ab01c061c865cd))
* **texteditor:**  some colors is incorrect. ([ba094484](https://github.com/linuxdeepin/deepin-editor/commit/ba094484a7c06bc0a3a3b38171f48e087ac9f785))
* **toast:**  possible memory leak ([4b01a349](https://github.com/linuxdeepin/deepin-editor/commit/4b01a349bd1d2dfc7e8297b68b3b5d2726d770fb))

#### Features

*   optimize emacs mode. ([dbc5d021](https://github.com/linuxdeepin/deepin-editor/commit/dbc5d0210eb55bb8923e28e262507ffcbcb82808))
*   sort theme list by lightness. ([8e15694c](https://github.com/linuxdeepin/deepin-editor/commit/8e15694cd0a97bd9cc4204846010132e917be4be))
*   add atom dark theme. ([f523c1aa](https://github.com/linuxdeepin/deepin-editor/commit/f523c1aa11f122b335b7ce4a1fdbec73c5dde75a))
*   support Ctrl + wheel to adjust font size. ([7873a610](https://github.com/linuxdeepin/deepin-editor/commit/7873a610cc974b4829de32a6967d45fd341b7e65))
*   delayed loadding highlight. ([4b0b87ea](https://github.com/linuxdeepin/deepin-editor/commit/4b0b87eac4f896766cdca32751016beb3d9b1b88))
*   close window prompt to save all files. ([b3b8cd9f](https://github.com/linuxdeepin/deepin-editor/commit/b3b8cd9f35c6469241971f7d548fd1b394c3c224))
*   support open srt files. ([9f33d6e8](https://github.com/linuxdeepin/deepin-editor/commit/9f33d6e82d6d1e0b37c7d8a5dd3906ee27481d25))
*   remove document margin. ([adb8ca94](https://github.com/linuxdeepin/deepin-editor/commit/adb8ca94f2db0cceec823ff9d60f981f14528db3))
*   intelligent judge whether to support comments. ([ead6f1d8](https://github.com/linuxdeepin/deepin-editor/commit/ead6f1d8cd7fab011faf0ae0ba3105542a896f3b))
*   support open bak files. ([54c6259c](https://github.com/linuxdeepin/deepin-editor/commit/54c6259cb50db2d1fd5725bb8bd62090ec09cb0b))
*   optimize theme panel selection effect. ([7b4cf4b6](https://github.com/linuxdeepin/deepin-editor/commit/7b4cf4b67ea72b4a9e75b8083e5073dc1bb31c02))
*   add solarized dark theme. ([22781ac6](https://github.com/linuxdeepin/deepin-editor/commit/22781ac60d503ba699e9d8661e5f055cd2d392a8))
*   prompt whether to save as when saving fails. ([b19cd092](https://github.com/linuxdeepin/deepin-editor/commit/b19cd092d63337be601dac986575a04b51dce6dc))
*   optimize load file ([adb356a5](https://github.com/linuxdeepin/deepin-editor/commit/adb356a521747329fe5b673e9093f7b5d69e5b26))
*   tabbar release changes window position. ([992f5415](https://github.com/linuxdeepin/deepin-editor/commit/992f5415158a8f3d2c7b7a89a10e8c9e62a7864e))
*   support multi-thraded read fiels. ([d4de7431](https://github.com/linuxdeepin/deepin-editor/commit/d4de743195567e7ee1b3211d92432645edc8bdc3))
*   change cursor when saving ([456230bc](https://github.com/linuxdeepin/deepin-editor/commit/456230bc42dd0f9f4c96ff3320f7e6dc882f5b7e))
*   add load mouse style. ([9ea1f8d7](https://github.com/linuxdeepin/deepin-editor/commit/9ea1f8d70e5682d7bbdb840fdcbeeab1c6112207))
*   add solarized-light theme ([3341f0fa](https://github.com/linuxdeepin/deepin-editor/commit/3341f0fa5b2c6ab15fddefa9d03590a2c358cd36))
*   can change the titlebar background color. ([37002495](https://github.com/linuxdeepin/deepin-editor/commit/37002495f56fbe21ee481a1c95754434f5459831))
*   optimize modification state. ([003db509](https://github.com/linuxdeepin/deepin-editor/commit/003db509fc12d519237b1c0afad022bc07ca7889))
*   align the top of the line numbers. ([4b0e41aa](https://github.com/linuxdeepin/deepin-editor/commit/4b0e41aadb235cd0f5314c2ec631ce5ee389033f))
*   support drag and drop the web url text. ([dc8413ab](https://github.com/linuxdeepin/deepin-editor/commit/dc8413ab10aaed5e0272b1d3472725801d4705a4))
*   save the file dialog history directory. ([1f28e873](https://github.com/linuxdeepin/deepin-editor/commit/1f28e873379a647239c30124e2da3cbd3e75e0b0))
*   update MimeType of desktop file ([0712e2fc](https://github.com/linuxdeepin/deepin-editor/commit/0712e2fcecf3e48526e02e1e12d41e0eff3540fd))
*   optimize startup speed. ([d7006a9c](https://github.com/linuxdeepin/deepin-editor/commit/d7006a9cece07800e0918d14863394516c756838))
*   select current directory when saving ([c370010f](https://github.com/linuxdeepin/deepin-editor/commit/c370010fde4e276d154effe96232cc6a966cae01))
*   supports drag files to window ([1af5c0f6](https://github.com/linuxdeepin/deepin-editor/commit/1af5c0f62b2347411508d44b918b8abfcdd7f424))
*   support open the desktop & yml files. ([1f6d3ae8](https://github.com/linuxdeepin/deepin-editor/commit/1f6d3ae8ace858ceb750c89f278f5785ac0f0d02))
*   save the correct newline. ([29e93136](https://github.com/linuxdeepin/deepin-editor/commit/29e93136aaf6633650e88f85ebefbca0383a4df6))
* **IndentText:**  optimize cursor blink. ([0175b85a](https://github.com/linuxdeepin/deepin-editor/commit/0175b85aaa771dcdc13a01e0ecf797f5b9f0704f))
* **TextEdit:**  auto adjust scrollbar margins. ([92559757](https://github.com/linuxdeepin/deepin-editor/commit/92559757731578a54abc8b4fad077d28d806e539))
* **ThemeView:**  auto adjust scrollbar margins. ([8e817ec4](https://github.com/linuxdeepin/deepin-editor/commit/8e817ec40627c8325f432ab8465bd4a9d3c3a621))
* **findbar:**  select all text when get focus. ([4a36520c](https://github.com/linuxdeepin/deepin-editor/commit/4a36520cb6c6a991dff8d6e8ed3464ae70814298))
* **printDialog:**  set output file name ([8d7d077d](https://github.com/linuxdeepin/deepin-editor/commit/8d7d077d9a1cda0ed95406b1cd86d73c159a1007))
* **save:**  correct toast content ([0c938f1f](https://github.com/linuxdeepin/deepin-editor/commit/0c938f1f3b849c2357dcc0db9fa568e3540dc388))
* **search:**  case insensitive. ([0a549e93](https://github.com/linuxdeepin/deepin-editor/commit/0a549e935565b767c882cac9df10cb311bd0658d))
* **tabbar:**  support background gradient. ([279dffa1](https://github.com/linuxdeepin/deepin-editor/commit/279dffa1199883bf9d4e3fae99884fdf49bee566))



### 0.0.5 (2018-05-23)
- Tag for archlinux.

### 0.0.4 (2018-05-17)
- Tag for archlinux.

### 0.0.3 (2018-05-14)
- Fixed many bugs since 0.0.2

### 0.0.2 (2018-05-10)
- First release for arch.

### 0.0.1 (2018-01-22)
- Init tag 
