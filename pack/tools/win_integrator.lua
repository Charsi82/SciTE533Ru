--[[
Windows Integrator
]]
require 'gui'
require 'shell'
require 'winreg'
do
	-- check for admin
	local hkey = winreg.openkey([[HKEY_LOCAL_MACHINE]],"w")
	if not hkey then
		shell.msgbox("Для изменения настроек требуется\nзапуск с правами администратора!","Ошибка", 0)
		return
	else
		hkey:close()
	end
end
local next_id = setmetatable({id = 0},{__call = function(self) self.id=self.id+1 return self.id end})
local wnd = gui.window("SciTE Windows Integrator 5.0b")
wnd:size(380, 430)
wnd:size(400, 400)
local desktop = gui.desktop()
desktop:center(wnd)
local callbacks = {}
local SetLang

-- переключалка интерфейса
local grbox1 = gui.groupbox(' Язык интерфейса: ')
wnd:add(grbox1,"top", 50)

-- интеграция в проводник
local grbox2 = gui.groupbox(' Интеграция в Проводник: ')
wnd:add(grbox2,"top", 175)

-- интеграция в Windows
local grbox3 = gui.groupbox(' Интеграция в Windows: ')
wnd:add(grbox3,"top", 100)

local RadioBtn1_ID = next_id()
local RadioBtn1 = gui.radiobutton("Английский", RadioBtn1_ID, true) -- caption, id, auto
wnd:add(RadioBtn1,"none")
RadioBtn1:position(15, 20)
callbacks[RadioBtn1_ID] = function(state) SetLang('eng') end

local RadioBtn2_ID = next_id()
local RadioBtn2 = gui.radiobutton("Русский", RadioBtn2_ID, true) -- caption, id, auto
wnd:add(RadioBtn2,"none")
RadioBtn2:position(150, 20)
RadioBtn2:check(1)
callbacks[RadioBtn2_ID] = function(state) SetLang('ru') end
if props['locale.properties'] == 'locale-ru.properties' then
	RadioBtn2:check(1)
else
	RadioBtn1:check(1)
end

local btnOK_ID = next_id()
local btnOK = gui.button("OK", btnOK_ID, true) -- def_push_button
wnd:add(btnOK, "none")
btnOK:size(70, 30)
btnOK:position(155, 330)
-- wnd:center_h(btnOK)

callbacks[btnOK_ID] = function() wnd:hide() end

local label1 = gui.label("Связать файлы заданных расширений с SciTE:", 0) -- 0 - text align left
wnd:add(label1, "none")
label1:position(15, 75)
label1:size(250, 20)

local cb3_posy = 100
local editbox = gui.editbox("txt;php;h;cxx;", 0x10, next_id()) --lowercase
wnd:add(editbox, "none")
editbox:position(37, cb3_posy)
editbox:size(250, 20)

local reg_backup = 'HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\SciTE\\Script\\WinIntegrator'
local function SetAssociations()
--[[	local hkey = winreg.openkey(reg_backup,"w")
	for ext in editbox:get_text():gmatch("%w+") do
		if ext~='' then
			local hkey_ext = winreg.openkey("HKCR\\."..ext)
			local current_association = hkey_ext:getstrval()
			print('for', ext, '=>', current_association)
			local hkey = winreg.openkey(reg_backup,"w")
			hkey:setvalue(ext, current_association)
		end
	end]]
	-- if hkey then hkey:setvalue("associations", editbox:get_text()) end
	editbox:enable(true)
end

local function UnSetAssociations()
	-- local hkey = winreg.openkey(reg_backup,"w")
	-- if hkey then hkey:deletevalue("associations", hkey) end
	editbox:enable(false)
end

local CheckBox1_ID = next_id()
local CheckBox1 = gui.checkbox(nil, CheckBox1_ID)
CheckBox1:position(15, cb3_posy)
CheckBox1:size(20, 20)
wnd:add(CheckBox1, "none")
callbacks[CheckBox1_ID] = function()
	local state = CheckBox1:check()
	if state == 1 then
		SetAssociations()
	else
		UnSetAssociations()
	end
end

local hkey = winreg.openkey(reg_backup)
if hkey then
	local associations = hkey:getvalue("associations")
	if associations then
		CheckBox1:check(1)
		editbox:set_text(associations)
		editbox:enable(true)
	else
		editbox:enable(false)
	end
end

local function SetSession()
	local hkey = winreg.createkey("HKCR\\.session","w")
	if hkey then hkey:setvalue("","SciTE.Session") end
	local hkey = winreg.createkey("HKCR\\SciTE.Session","w")
	if hkey then
		hkey:setvalue("","SciTE session file")
	end
	local hkey = winreg.createkey("HKCR\\SciTE.Session\\DefaultIcon","w")
	if hkey then
		hkey:setvalue("",props['SciteDefaultHome'].."\\SciTE.exe,2")
	end
	local hkey = winreg.createkey("HKCR\\SciTE.Session\\shell\\open\\command","w")
	if hkey then
		hkey:setvalue("",'cmd /v:on /c "set "file=%1" && start "" "'..
		props['SciteDefaultHome']:gsub("\\","\\\\")..
		[[\SciTE.exe" -check.if.already.open=0 "-loadsession:!file:\=\\!"]])
	end
	--cmd /v:on /c "set "file=%1" && start "" "G:\\Program Files (x86)\\SciTE\\SciTE.exe" -check.if.already.open=0 "-loadsession:!file:\=\\!"
end

local function UnsetSession()
	local hkey = winreg.openkey("HKCR", "w")
	hkey:deletekey(".session")
end

local CheckBox2_ID = next_id()
local CheckBox2 = gui.checkbox('Открывать файлы *.session как файлы сессий SciTE', CheckBox2_ID)
CheckBox2:position(15, cb3_posy + 30)
wnd:add(CheckBox2,"none")
callbacks[CheckBox2_ID] = function()
	local state = CheckBox2:check()
	if state == 1 then
		SetSession()
	else
		UnsetSession()
	end
end
local hkey = winreg.openkey([[HKCR\.session]])
if hkey and hkey:getvalue("")=="SciTE.Session" then
	CheckBox2:check(1)
end

local CheckBox3_ID = next_id()
local CheckBox3 = gui.checkbox('Добавить SciTE в контекстное меню "Отправить"', CheckBox3_ID)
CheckBox3:position(15, cb3_posy + 60)
wnd:add(CheckBox3,"none")
local send_to = gui.get_folder(9)
callbacks[CheckBox3_ID] = function()
	local state = CheckBox3:check()
	local send_to = gui.get_folder(9)
	if state == 1 then
		gui.create_link(props['SciteDefaultHome']..[[\SciTE.exe]], send_to..[[\SciTE.lnk]])
	else
		os.remove(send_to..[[\SciTE.lnk]])
	end
end
CheckBox3:check(shell.fileexists(send_to..[[\SciTE.lnk]]) and 1 or 0)

local CheckBox4_ID = next_id()
local CheckBox4 = gui.checkbox("Установить SciTE в качестве дефолтового HTML редактора", CheckBox4_ID)
CheckBox4:position(15, cb3_posy + 90)
wnd:add(CheckBox4,"none")
local send_to = gui.get_folder(9)
callbacks[CheckBox4_ID] = function()
	local state = CheckBox4:check()
	local hkey = winreg.openkey([[HKCR\.htm\OpenWithProgids]], "w")
	local hkey2 = winreg.openkey([[HKCR\.html\OpenWithProgids]], "w")
	if state == 1 then
		hkey:setvalue("SciTE.file", '')
		hkey2:setvalue("SciTE.file", '')
	else
		hkey:deletevalue("SciTE.file")
		hkey2:deletevalue("SciTE.file")
	end
end
local hkey = winreg.openkey([[HKCR\.htm\OpenWithProgids]])
CheckBox4:check( (hkey:getstrval("SciTE.file") == '') and 1 or 0 )

-- Интеграция в Windows
local label2 = gui.label("Установить SciTE.Helper (COM-сервер для управления SciTE)", 0)
wnd:add(label2, "none")
label2:position(15, 250)
label2:size(400, 30)

-- register Helper
local RegSrvBtn_ID = next_id()
local RegSrvBtn = gui.button("Зарегистрировать", RegSrvBtn_ID)
wnd:add(RegSrvBtn, "none")
RegSrvBtn:position(50,278)
RegSrvBtn:size(110,30)
callbacks[RegSrvBtn_ID] = function()
	local cmd = string.format('regsvr32 /s "%s%s"', props['SciteDefaultHome'], [[\tools\Helper\SciTE.dll]])
	local res = {os.execute(cmd)}
	if not res[1] then
		wnd:status_setpart(0, "Registeration failed. Run SciTE as Admin.")
	else
		wnd:status_setpart(0, "Registeration success.")
	end
end

-- unregister Helper
local UnRegSrvBtn_ID = next_id()
local UnRegSrvBtn = gui.button("Удалить", UnRegSrvBtn_ID)
wnd:add(UnRegSrvBtn, "none")
UnRegSrvBtn:position(210,278)
UnRegSrvBtn:size(110,30)
callbacks[UnRegSrvBtn_ID] = function()
	local cmd = string.format('regsvr32 /u /s "%s%s"', props['SciteDefaultHome'], [[\tools\Helper\SciTE.dll]])
	-- print(os.execute(cmd))
	local res = {os.execute(cmd)}
	if not res[1] then
		wnd:status_setpart(0, "Unregisteration failed. Run SciTE as Admin.")
	else
		wnd:status_setpart(0, "Unregisteration success.")
	end
end

wnd:on_command(function(cmd_id, state)
	local cb = callbacks[cmd_id]
	if cb then cb() end
end)

wnd:statusbar(-1)
function SetLang(lng_id)
	local str_ids = 
	{
		['eng'] = {
			grbox1 = " Interface Language: ",
			regsrv = "Register",
			unregsrv = "Unregister",
			intgr_expl = " Integration with Explorer: ",
			intgr_win = " Integration with Windows: ",
			label_helper = "Install SciTE.Helper (COM-server for communication with SciTE)",
			st_bar = "Administrator rights required.",
			label_bind = "Associate SciTE with extensions:",
			check_sess = "Associate files *.session as SciTE session files",
			check_sendto = 'Add SciTE to "SendTo" context menu',
			check_html_def = "Set SciTE as default HTML editor",
			lang_tip = "Change interface this application. SciTE Interface will change after restart",
			check1_tip = "You\'ll able to open in SciTE files with this extensions by double click",
			check2_tip = "You\'ll able to open SciTE session files by double click",
			check3_tip = "You\'ll able to open any selected files with SciTE",
			check4_tip = 'Adds SciTE to the "Open With.." menu for html files',
		},
		['ru'] = {
			grbox1 = " Язык интерфейса: ",
			regsrv = "Зарегистрировать",
			unregsrv = "Удалить",
			intgr_expl = " Интеграция в Проводник: ",
			intgr_win = " Интеграция в Windows: ",
			label_helper = "Установить SciTE.Helper (COM-сервер для управления SciTE)",
			st_bar = "Требуются права Администратора.",
			label_bind = "Связать файлы заданных расширений с SciTE:",
			check_sess = "Открывать файлы *.session как файлы сессий SciTE",
			check_sendto = 'Добавить SciTE в контекстное меню "Отправить"',
			check_html_def = "Установить SciTE в качестве дефолтового HTML редактора",
			lang_tip = "Сменить интерфейс этого приложения. Интерфейс SciTE изменится после перезапуска",
			check1_tip = "Позволяет открывать в SciTE файлы ассоциированных типов с помощью двойного клика мыши",
			check2_tip = "Позволяет открывать сохраненные ранее сессиии SciTE",
			check3_tip = "Позволяет открывать в SciTE любые выбранные файлы",
			check4_tip = 'Добавляет SciTE в меню "Открыть с помощью..". для html файлов',
		}
	}
	local lng = str_ids[lng_id]
	if lng then
		grbox1:set_text( lng['grbox1'] or "" )
		wnd:tooltip(RadioBtn1_ID, lng['lang_tip'] or "")
		wnd:tooltip(RadioBtn2_ID, lng['lang_tip'] or "")
		wnd:tooltip(CheckBox1_ID, lng['check1_tip'] or "")
		wnd:tooltip(CheckBox2_ID, lng['check2_tip'] or "")
		wnd:tooltip(CheckBox3_ID, lng['check3_tip'] or "")
		wnd:tooltip(CheckBox4_ID, lng['check4_tip'] or "")

		grbox2:set_text(lng['intgr_expl'] or "")
		label1:set_text(lng['label_bind'] or "")
		CheckBox2:set_text(lng['check_sess'] or "")
		CheckBox3:set_text(lng['check_sendto'] or "")
		CheckBox4:set_text(lng['check_html_def'] or "")

		grbox3:set_text(lng['intgr_win'] or "")
		label2:set_text(lng['label_helper'] or "")
		RegSrvBtn:set_text(lng['regsrv'] or "")
		UnRegSrvBtn:set_text(lng['unregsrv'] or "")
		wnd:status_setpart(0, lng['st_bar'] or "")
	end
end

SetLang( (props['locale.properties']=='locale-ru.properties') and 'ru' or 'eng' )
wnd:show(true)

-- test
--[[local btn = gui.button("test", 158)
wnd:add(btn,"none")
btn:size(50,50)
-- btn:position(0,0)
--400-50
-- RegSrvBtn:center_h(btn)
-- RegSrvBtn:center_v(btn)
-- RegSrvBtn:center(btn)
-- wnd:center(btn)
print(btn:position())
--(380-50)/2 (430-50)/2 165 190]]



























