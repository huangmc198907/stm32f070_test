

class "IotStateView"(QFrame)

function IotStateView:__init(socket)
    QFrame.__init(self)
    self.socket = socket

    self.levelImg = {QImage(), QImage(), QImage()}
    self.levelImg[1]:load("./level.png")
    self.levelImg[2]:load("./levelLow.png")
    self.levelImg[3]:load("./levelHigh.png")

    self.levelLowImg = {QImage(), QImage()}
    self.levelLowImg[1]:load("./levelLowNormal.png")
    self.levelLowImg[2]:load("./levelLowLow.png")

    --logEdit:append(""..img.width.." "..img.height)
    --self.lab = QLabel()
    --self.lab.pixmap = QPixmap.fromImage(img:scaled(60, 102))

    self.relayStateString = {"OFF", "ON"}

    self.relayState = {}
    self.current = {}
    self.levelState = {}

    for i=1,4 do
        self.relayState[#self.relayState + 1] = QLabel{text = self.relayStateString[1]}
    end

    for i=1,4 do
        self.levelState[#self.levelState + 1] = QLabel()
        if i <=2 then
            self.levelState[#self.levelState].pixmap = QPixmap.fromImage(self.levelLowImg[1]:scaled(self.levelLowImg[1].width/2, self.levelLowImg[1].height/2))
        else
            self.levelState[#self.levelState].pixmap = QPixmap.fromImage(self.levelImg[1]:scaled(self.levelImg[1].width/2, self.levelImg[1].height/2))
        end
    end

    for i=1,3 do
        self.current[#self.current + 1] = QLabel{text = "0"}
    end

    self.relayStateGroup = QGroupBox("Relay State"){ layout = QVBoxLayout{
				QHBoxLayout{
					QVBoxLayout{QLabel("药罐1继电器"), self.relayState[1]},
					QVBoxLayout{QLabel("药罐2继电器"), self.relayState[2]},
					QVBoxLayout{QLabel("水泵1继电器"), self.relayState[3]},
                    QVBoxLayout{QLabel("水泵2继电器"), self.relayState[4]},
                    QLabel(""), strech = "0,0,0,0,1",
				},
                QLabel(""), strech = "0,1",
            }, }

    self.levelStateGroup = QGroupBox("Level State"){ layout = QVBoxLayout{
                QHBoxLayout{
                    QVBoxLayout{QLabel("药罐1低液位浮球"), self.levelState[1]},
                    QVBoxLayout{QLabel("药罐2低液位浮球"), self.levelState[2]},
                    QVBoxLayout{QLabel("水槽1液位"), self.levelState[3]},
                    QVBoxLayout{QLabel("水槽1液位"), self.levelState[4]},
                    QLabel(""), strech = "0,0,0,0,1",
                },
                QLabel(""), strech = "0,1",
            }, }

    self.currentGroup = QGroupBox("Current (mA)"){ layout = QVBoxLayout{
                QHBoxLayout{
                    QVBoxLayout{QLabel("风机电流"), self.current[1]},
                    QVBoxLayout{QLabel("模拟输入1"), self.current[2]},
                    QVBoxLayout{QLabel("模拟输入2"), self.current[3]},
                    QLabel(""), strech = "0,0,0,1",
                },
                QLabel(""), strech = "0,1",
            }, }        

    self.layout = QVBoxLayout{
            self.relayStateGroup,
            self.levelStateGroup,
            self.currentGroup,
            QLabel(""), strech = "0,0,0,1",
        }
end

class "IotRegView"(QFrame)

local readRegAddr = {
    0x00000000, 0x00000008, 0x0000000c, 0x00000010, 0x00000014, 0x01000100, 0x01000110, 0x01000120, 
    0x01000122, 0x00010000, 0x00010001, 0x00010002, 0x00010004, 0x00010006, 0x01010000, 0x01010002, 
    0x01010004, 0x01010006,
}
local readRegAddrString = {
    "ID", "Hard Ver", "Soft Ver", "Net Type", "RTC", "FTP User", "FTP Pwd", "FPT Port", 
    "FTP Host", "Relay State", "Level state", "fan Current", "Current 1", "Current 2", "药泵1", "药泵2",
    "水泵1", "水泵2",
}

local writeRegAddr = {
    0x01000014, 0x01000100, 0x01000110, 0x01000120, 0x01000122, 0x01010000, 0x01010002, 0x01010004, 
    0x01010006,
}
local writeRegAddrString = {
    "RTC", "FTP User", "FTP Pwd", "FPT Port", "FTP Host", "药泵1", "药泵2", "水泵1", 
    "水泵2",
}

function IotRegView:__init(socket)
    QFrame.__init(self)
    self.socket = socket

    self.devErrText = QLineEdit{ text = "", readOnly = true }
    self.devReadRegBtn = QPushButton("Read REG")
    self.readAddrSelectBox = QCheckBox(self){checked=true}
    self.readAddrSelectLabel = QLabel{text = "Select"}
    self.readAddrSelectText = QComboBox{readRegAddrString, editable = false, hidden = false}
    self.readAddrText = QLineEdit{text="", inputMask = "0xHHHHHHHH", readOnly = true}
    self.readLenText = QLineEdit{ text = "8", inputMask = "99999"}
    self.readHexEdit = QHexEdit{
        overwriteMode = false,
        readOnly = true,
    }
    self.readAddrText.text = string.format("0x%08x", readRegAddr[self.readAddrSelectText.currentIndex + 1])

    self.devWriteRegBtn = QPushButton("Write REG")
    self.writeAddrSelectBox = QCheckBox(self){checked=true}
    self.writeAddrSelectLabel = QLabel{text = "Select"}
    self.writeAddrSelectText = QComboBox{writeRegAddrString, editable = false, hidden = false}
    self.writeAddrText = QLineEdit{text="", inputMask = "0xHHHHHHHH", readOnly = true}
    self.writeHexEdit = QHexEdit{
        overwriteMode = false,
        readOnly = false,
    }
    self.writeAddrText.text = string.format("0x%08x", writeRegAddr[self.writeAddrSelectText.currentIndex + 1])

    self.regReadGroup = QGroupBox("Read"){ 
        layout = QVBoxLayout{
            QHBoxLayout{ 
                self.readAddrSelectBox, self.readAddrSelectLabel, self.readAddrSelectText, QLabel("Addr:"), self.readAddrText,
                QLabel("Len:"), self.readLenText, QLabel(""), strech = "0,0,0,0,0,0,0,1", 
            },
            QHBoxLayout{ 
                self.devReadRegBtn, QLabel(""), strech = "0,1", 
            },
            self.readHexEdit,
            QLabel(""), strech = "0,0,0,1",
        },
    }

    self.regWriteGroup = QGroupBox("Write"){ 
        layout = QVBoxLayout{
            QHBoxLayout{ 
                self.writeAddrSelectBox, self.writeAddrSelectLabel, self.writeAddrSelectText, QLabel("Addr:"), self.writeAddrText,
                QLabel(""), strech = "0,0,0,0,0,1", 
            },
            QHBoxLayout{ 
                self.devWriteRegBtn, QLabel(""), strech = "0,1", 
            },
            self.writeHexEdit,
            QLabel(""), strech = "0,0,0,1",
        },
    }

    self.regReadAndWriteGroup = QGroupBox("Read or Write Register"){ 
            layout = QVBoxLayout{
                QHBoxLayout{

                    QLabel("Control Result:"), self.devErrText, QLabel(""), strech = "0,0,1",
                },
                self.regReadGroup,
                self.regWriteGroup,
                QLabel(""), strech = "0,0,0,1",
            },
        }

    self.layout = QVBoxLayout{
            self.regReadAndWriteGroup,
            QLabel(""), strech = "0,1",
        }

    self.readAddrSelectBox.clicked = function()
        if self.readAddrSelectBox.checked then
            self.readAddrSelectText.hidden = false
            self.readAddrSelectLabel.text = "Select"
            self.readAddrText.readOnly = true
            self.readAddrText.text = string.format("0x%08x", readRegAddr[self.readAddrSelectText.currentIndex + 1])
        else
            self.readAddrSelectText.hidden = true
            self.readAddrSelectLabel.text = "Input"
            self.readAddrText.readOnly = false
        end
    end

    self.writeAddrSelectBox.clicked = function()
        if self.writeAddrSelectBox.checked then
            self.writeAddrSelectText.hidden = false
            self.writeAddrSelectLabel.text = "Select"
            self.writeAddrText.readOnly = true
            self.writeAddrText.text = string.format("0x%08x", writeRegAddr[self.writeAddrSelectText.currentIndex + 1])
        else
            self.writeAddrSelectText.hidden = true
            self.writeAddrSelectLabel.text = "Input"
            self.writeAddrText.readOnly = false
        end
    end

    self.readAddrSelectText.currentIndexChanged = function()
        self.readAddrText.text = string.format("0x%08x", readRegAddr[self.readAddrSelectText.currentIndex + 1])
    end  

    self.writeAddrSelectText.currentIndexChanged = function()
        self.writeAddrText.text = string.format("0x%08x", writeRegAddr[self.writeAddrSelectText.currentIndex + 1])
    end 
end

class "IotInfoView"(QFrame)

function IotInfoView:__init(socket)
    QFrame.__init(self)
    self.socket = socket

    self.relayString = {"药泵1", "药泵2", "水泵1", "水泵2"}
    self.relayCtrlString = {"OFF","ON","Run On time"}

    self.devSNText = QLineEdit{ text = "", readOnly = true }
    self.devNetTypeText = QLineEdit{ text = "", readOnly = true }
    self.hardVerText = QLineEdit{ text = "", readOnly = true }
    self.softVerVerText = QLineEdit{ text = "", readOnly = true }
    self.heartAddrText = QLineEdit{ text = "", readOnly = true }
    self.heartLenText = QLineEdit{ text = "", readOnly = true }
    self.heartSeqText = QLineEdit{ text = "", readOnly = true }
    self.devSetRTCBtn = QPushButton("Set RTC")
    self.rtcSelectBox = QCheckBox(self){checked=true}
    self.devSetRTCLabel = QLabel{text = "Auto"}
    self.devSetRTCEdit = QLineEdit{text = "", inputMask = "9999999999", readOnly = true}
    self.ftpUserEdit = QLineEdit{text = ""}
    self.ftpPwdEdit = QLineEdit{text = ""}
    self.ftpPortEdit = QLineEdit{text = "", inputMask = "99999"}
    self.ftpHostEdit = QLineEdit{text = ""}
    self.ftpSetBtn = QPushButton("Set FTP Info")
    self.selecRelayBox = QComboBox{self.relayString, editable = false}
    self.relayCtrlBox = QComboBox{self.relayCtrlString, editable = false}
    self.relayTimeEdit = QLineEdit{text = "", inputMask = "99999", hidden = true}
    self.relayBtn = QPushButton("Relay Control")
    --self.devSetRTCText.text = "Input"

    self.devSetRTCEdit.text = ""..os.time()

    self.heartGroup = QGroupBox("Heart:"){ 
            layout = QHBoxLayout{
                QLabel("Addr:"), self.heartAddrText,
                QLabel("Len:"), self.heartLenText,
                QLabel("Seq:"), self.heartSeqText, 
                QLabel(""), strech = "0,0,0,0,0,0,1",
            }, 
        }

    self.ftpGroup = QGroupBox("FTP:"){ 
            layout = QVBoxLayout{
                QHBoxLayout{
                    QLabel("Host:"), self.ftpHostEdit, 
                    QLabel("Port:"), self.ftpPortEdit,
                    strech = "0,1,0,0",
                },
                QHBoxLayout{
                    QLabel("User:"), self.ftpUserEdit,
                    QLabel("Password:"), self.ftpPwdEdit,
                    QLabel(""), strech = "0,0,0,0,1",
                },
                self.ftpSetBtn,
                QLabel(""), strech = "0,0,0,1",
            }, 
        }        

    self.layout = QVBoxLayout{
            QHBoxLayout{
                QLabel("SN:"), self.devSNText,
                QLabel("NET Type:"), self.devNetTypeText,
                QLabel(""), strech = "0,1,0,0",
            },
            QHBoxLayout{
                QLabel("Hard Ver:"), self.hardVerText,
                QLabel("Soft ver:"), self.softVerVerText, 
                QLabel(""), strech = "0,0,0,0,1",
            },
            self.heartGroup,
            QHBoxLayout{
                self.devSetRTCBtn, self.rtcSelectBox, self.devSetRTCLabel, self.devSetRTCEdit,
                QLabel(""), strech = "0,0,0,0,1",
            },
            self.ftpGroup,
            QHBoxLayout{
                QLabel("Relay Control"), self.selecRelayBox, self.relayCtrlBox, self.relayTimeEdit, self.relayBtn,
                QLabel(""), strech = "0,0,0,0,0,1",
            },
            QLabel(""), strech = "0,0,0,0,0,0,1",
        }

    self.rtcSelectBox.clicked = function()
        if self.rtcSelectBox.checked then
            self.devSetRTCLabel.text = "Auto"
            self.devSetRTCEdit.readOnly = true
            local time = os.time()
            self.devSetRTCEdit.text = ""..time
            logEdit:append("date:"..time.."(".. os.date("%Y/%m/%d %H:%M:%S",time) ..")")
        else
            self.devSetRTCLabel.text = "Input"
            self.devSetRTCEdit.readOnly = false
            --local time = self.devSetRTCEdit.text
            --logEdit:append("date:"..time.."(".. os.date("%Y/%m/%d %H:%M:%S",time) ..")")
        end
    end
    self.relayCtrlBox.currentIndexChanged = function()
        logEdit:append("current relay control:"..self.relayCtrlBox.currentText)
        if self.relayCtrlBox.currentText == self.relayCtrlString[3] then
            self.relayTimeEdit.hidden = false
        else
            self.relayTimeEdit.hidden = true
        end
    end    
end

local STX=(0x02)      --报文开始
local ETX=(0x03)      --报文结束
local DLE=(0x10)      --转义字符
local NAK=(0x15)      --检测连接状态时使用
local ACK=(0x06)      --ACK

local DEV_TYPE=(0x0001)            --设备类型 污水控制器

local function CMD_GET(cmd)
    return QUtil.bitand(cmd, 0x7f)  --服务器到设备为 0x80 | cmd
end

local function CMD_DIRECTION(cmd)
    return QUtil.bitand(cmd, 0x80)  --获取命令发起方向
end

local CMD_FROME_SERVER=(0x00)                  --命令为服务器发送
local CMD_BACK_FROME_SERVER=(0x80)                  --命令为服务器回复命令

local function CMD_BACK(cmd)
    return QUtil.bitor(cmd, 0x80)  --生成回复命令
end

local CMD_REGISTER=(0x01)              --设备注册命令
local CMD_HEART=(0x02)              --设备心跳
local CMD_RESTART=(0x13)              --设备重启
local CMD_READ=(0x14)              --读取数据命令
local CMD_WRITE=(0x15)              --写入数据命令
local CMD_REQUEST=(0x06)              --请求数据命令

local NET_2G=(0x01)              --设备使用2G网络
local NET_WIFI=(0x02)              --设备使用WIFI
local NET_4G=(0x04)              --设备使用4G网络
local NET_5G=(0x08)              --设备使用5G网络
local NET_LINE=(0x10)              --设备使用有线网络

class "IotView"(QFrame)

local netTypeString = { "2G", "WIFI", "4G", "5G", "Line"}

function IotView:__init(server_client_view)
    QFrame.__init(self)

    self.tab = QTabWidget(self){
        tabsClosable = true,
        movable = true,

    }

    self.info = IotInfoView(self.socket)

    self.reg = IotRegView(self.socket)

    self.state = IotStateView(self.socket)

    self.server_client_view = server_client_view
    self.socket = server_client_view.socket

    self.layout = QVBoxLayout{
            self.tab,
            QLabel(""), strech = "0,1",
        }

    self.step = 1
    self.data = ""
    self.bcc = 0
    self.maxMotors = 15

    self.showData = function(string, data)
        local dataString = ""
        if data and type(data) == "table" then
            logEdit:append("table::")
            for i=1,#data do
                dataString = dataString..string.format("%02x ", QUtil.bitand(data[i], 0xff))
            end
            logEdit:append((string or "")..dataString)
        elseif data and type(data) == "string" then
            logEdit:append("string::")
            for i=1,#data do
                dataString = dataString..string.format("%02x ", data:byte(i))
            end
            logEdit:append((string or "")..dataString)
        end
    end

    self.parse_register = function(data)
        self.showData("register data:", data)
        local id = 1
        local len = data:byte(1) + data:byte(2) * 256
        id = id + 2
        logEdit:append("recv len:"..len.." need len:25")
        if len == 25 then
            local net = data:byte(3)
            logEdit:append("net:"..string.format("%08x", net))
            local netString = ""
            for i=1,#netTypeString do
                if QUtil.bitand(net, 0x1) == 1 then
                    netString = netString..netTypeString[i]
                end
                net = QUtil.bitand(net/2, 0xffffffff)
            end
            logEdit:append("net string:"..netString)
            id = id + 1
            local sn = ""
            for i=1,8 do
                sn = sn..string.format("%02x ", data:byte(3+i))
                id = id + 1
            end
            logEdit:append("sn:"..sn.."("..data:sub(4,12)..")")
            local hardVer = data:byte(id) + data:byte(id + 1)*0x100 + data:byte(id + 2)*0x10000 + data:byte(id + 3)*0x1000000
            logEdit:append("hard version:"..string.format("%08x", hardVer))
            id = id + 4
            local softVer = data:byte(id) + data:byte(id + 1)*0x100 + data:byte(id + 2)*0x10000 + data:byte(id + 3)*0x1000000
            logEdit:append("soft version:"..string.format("%08x", softVer))
            id = id + 4
            local heartAddr = data:byte(id) + data:byte(id + 1)*0x100 + data:byte(id + 2)*0x10000 + data:byte(id + 3)*0x1000000
            logEdit:append("heart address:"..string.format("%08x", heartAddr))
            id = id + 4
            local heartLen = data:byte(id) + data:byte(id + 1)*0x100 + data:byte(id + 2)*0x10000 + data:byte(id + 3)*0x1000000
            logEdit:append("heart len:"..string.format("%08x", heartLen))

            self.tab:addTab(self.info, "Info view")
            self.tab:setCurrentWidget(self.info)

            self.tab:addTab(self.reg, "Reg view")
            self.tab:setCurrentWidget(self.reg)

            self.tab:addTab(self.state, "State view")
            self.tab:setCurrentWidget(self.state)

            self.info.devSNText.text = sn
            self.info.devNetTypeText.text = netString
            self.info.hardVerText.text = string.format("%08x", hardVer)
            self.info.softVerVerText.text = string.format("%08x", softVer)
            self.info.heartAddrText.text = string.format("%08x", heartAddr)
            self.info.heartLenText.text = ""..heartLen
        end
    end

    self.parse_heart = function(data)
        self.showData("heart data:", data)
        local len = data:byte(1) + data:byte(2) * 256
        local heartLen = tonumber(self.info.heartLenText.text) or 0
        logEdit:append("recv len:"..len.." heart len:"..heartLen)
        local id=3
        if len == heartLen + 16 then
            local sn = ""
            for i=1,8 do
                sn = sn..string.format("%02x ", data:byte(id))
                id = id + 1
            end
            logEdit:append("recv sn:"..sn.."(register sn:"..self.info.devSNText.text..")")
            local heartSeq = data:byte(id) + data:byte(id + 1)*0x100 + data:byte(id + 2)*0x10000 + data:byte(id + 3)*0x1000000
            logEdit:append("heart sequence:"..string.format("%08x", heartSeq))
            id = id + 4
            local heartAddr = data:byte(id) + data:byte(id + 1)*0x100 + data:byte(id + 2)*0x10000 + data:byte(id + 3)*0x1000000
            logEdit:append("heart address:"..string.format("%08x", heartAddr))
            id = id + 4
            if self.info.devSNText.text:match(sn) then
                local relayState = data:byte(id)
                logEdit:append("relay state:"..string.format("%04x", relayState))
                id = id + 1
                local levelState = data:byte(id)
                logEdit:append("level state:"..string.format("%04x", levelState))
                id = id + 1
                local current = {}
                current[#current + 1] = QUtil.bitand(data:byte(id) + data:byte(id + 1)*0x100, 0xffff)
                logEdit:append("fan current:"..string.format("%04x", current[#current]))
                id = id + 2
                current[#current + 1] = QUtil.bitand(data:byte(id) + data:byte(id + 1)*0x100, 0xffff)
                logEdit:append("current 1:"..string.format("%04x", current[#current]))
                id = id + 2
                current[#current + 1] = QUtil.bitand(data:byte(id) + data:byte(id + 1)*0x100, 0xffff)
                logEdit:append("current 2:"..string.format("%04x", current[#current]))
                id = id + 2

                self.info.heartSeqText.text = ""..heartSeq
                
                for i=1,#self.state.relayState do
                    if 0 == QUtil.bitand(relayState, 0x1) then
                        self.state.relayState[i].text = self.state.relayStateString[1]
                    else
                        self.state.relayState[i].text = self.state.relayStateString[2]
                    end
                    relayState = QUtil.bitand(relayState/2, 0xff)
                end

                local w = self.state.levelLowImg[1].width/2
                local h = self.state.levelLowImg[1].height/2
                for i=1,#self.state.levelState do
                    if i <= 2 then
                        if 0 == QUtil.bitand(levelState, 0x1) then
                            self.state.levelState[i].pixmap = QPixmap.fromImage(self.state.levelLowImg[1]:scaled(w, h))
                        else
                            self.state.levelState[i].pixmap = QPixmap.fromImage(self.state.levelLowImg[2]:scaled(w, h))
                        end
                        relayState = QUtil.bitand(relayState/2, 0xff)
                    else
                        if 0 == QUtil.bitand(levelState, 0x1) and 0 == QUtil.bitand(levelState, 0x10) then
                            self.state.levelState[i].pixmap = QPixmap.fromImage(self.state.levelImg[1]:scaled(w, h))
                        elseif 0 ~= QUtil.bitand(levelState, 0x1) and 0 == QUtil.bitand(levelState, 0x10) then
                            self.state.levelState[i].pixmap = QPixmap.fromImage(self.state.levelImg[2]:scaled(w, h))
                        elseif 0 == QUtil.bitand(levelState, 0x1) and 0 ~= QUtil.bitand(levelState, 0x10) then
                            self.state.levelState[i].pixmap = QPixmap.fromImage(self.state.levelImg[3]:scaled(w, h))
                        end
                        relayState = QUtil.bitand(relayState/2, 0xff)
                    end
                end
                for i=1,#self.state.current do
                    self.state.current[i].text = ""..current[i]
                end
            end
        end
    end

    self.parse_request = function(data)
        self.showData("request data:", data)
        local len = data:byte(1) + data:byte(2) * 256
    end

    self.parse_data = function(data)
        self.showData("parse data:", data)
        local devType = data:byte(1) + data:byte(2) * 256
        if devType == DEV_TYPE then
            local cmd = data:byte(3)
            if cmd == CMD_REGISTER then
                self.parse_register(data:sub(4, #data))
            elseif cmd == CMD_HEART then
                self.parse_heart(data:sub(4, #data))
            elseif cmd == CMD_REQUEST then
                self.parse_request(data:sub(4, #data))
            end
        end
    end

    self.server_client_view.readyRead = function(data)
        if data and #data > 0 then
            self.showData("recv data:", data)
            for i=1,#data do
                local tmp = QUtil.bitand(data[i], 0xff)
                if self.step == 1 then
                    if tmp == STX then
                        self.step = self.step+1
                        self.bcc = 0
                        self.data = ""
                    end
                elseif self.step == 2 then
                    if tmp == DLE then
                        self.step = self.step+1
                    elseif tmp == ETX then
                        self.step = self.step+2
                    else
                        self.data = self.data..string.char(tmp)
                        self.bcc = QUtil.bitand(QUtil.bitxor(self.bcc, tmp), 0xff)
                    end
                elseif self.step == 3 then
                    if tmp == STX or tmp == ETX or tmp == DLE or tmp == ACK or tmp == NAK then
                        self.data = self.data..string.char(tmp)
                        self.bcc = QUtil.bitand(QUtil.bitxor(self.bcc, tmp), 0xff)
                        self.step = self.step-1
                    else
                        self.step = 1
                        logEdit:append("data DLE error:"..string.format("%02x", tmp))
                    end
                elseif self.step == 4 then
                    self.step = 1
                    if tmp == self.bcc then
                        self.parse_data(self.data)
                        if i < #data then
                            local tmpdata = {}
                            for k=i,#data do
                                tmpdata[#tmpdata + 1] = data[k]
                            end
                            self.server_client_view.readyRead(tmpdata)
                        end
                    else
                        logEdit:append("bcc error"..string.format(" bcc=%02x, data=%02x",self.bcc,tmp))
                    end
                end    
            end
        end
        --logEdit:append("Server socket ready read")
    end
end
