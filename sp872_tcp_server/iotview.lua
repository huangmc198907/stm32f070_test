

class "IotStateView"(QFrame)

function IotStateView:__init(number, socket)
    QFrame.__init(self)
    self.number = number
    self.socket = socket

    self.levelImg = {QImage(), QImage(), QImage()}
    self.levelImg[1]:load("./level.png")
    self.levelImg[2]:load("./levelHigh.png")
    self.levelImg[3]:load("./levelLow.png")

    --logEdit:append(""..img.width.." "..img.height)
    --self.lab = QLabel()
    --self.lab.pixmap = QPixmap.fromImage(img:scaled(60, 102))

    self.motorStateString = {"Stop", "Run", "Error"}
    self.motorErrNumberString = {"No"}

    self.motorState = {}
    self.motorErrNumber = {}
    self.motorCurrent = {}
    self.levelState = {}

    self.motorStateSubGroup = {}
    self.motorStateSubGroup[1] = QGroupBox("  "){ 
        layout = QVBoxLayout{ 
            QLabel("State        ->"), QLabel("Current      ->"), QLabel("Error Number ->"),
        },
    }
    for i=1,17 do
        self.motorState[#self.motorState + 1] = QLabel{text = self.motorStateString[1]}
        self.motorErrNumber[#self.motorErrNumber + 1] = QLabel{text = self.motorErrNumberString[1]}
        self.motorCurrent[#self.motorCurrent + 1] = QLabel{text = "0"}
        if i <= self.number then
            self.motorStateSubGroup[#self.motorStateSubGroup + 1] = QGroupBox(""..(i-1)){ 
                layout = QVBoxLayout{ 
                    self.motorState[i], self.motorCurrent[i], self.motorErrNumber[i],
                },
            }
        else
            self.motorStateSubGroup[#self.motorStateSubGroup + 1] = QGroupBox(""..(i-1)){ 
                layout = QVBoxLayout{ 
                    self.motorState[i], self.motorCurrent[i], self.motorErrNumber[i],
                },
                hidden = true,
            }
        end
    end

    self.levelStateSubGroup = {}
    for i=1,8 do
        self.levelState[#self.levelState + 1] = QLabel()
        self.levelState[#self.levelState].pixmap = QPixmap.fromImage(self.levelImg[1]:scaled(self.levelImg[1].width/2, self.levelImg[1].height/2))
        self.levelStateSubGroup[#self.levelStateSubGroup + 1] = QGroupBox(""..(i-1)){ 
            layout = QVBoxLayout{ 
                self.levelState[i],
            },
        }
    end

    self.motorStateGroup = QGroupBox("Motor Run State"){ layout = QVBoxLayout{
				QHBoxLayout{
					self.motorStateSubGroup[1],
					self.motorStateSubGroup[2],
					self.motorStateSubGroup[3],
					self.motorStateSubGroup[4],
					self.motorStateSubGroup[5],
					self.motorStateSubGroup[6],
					self.motorStateSubGroup[7],
					self.motorStateSubGroup[8],
					self.motorStateSubGroup[9],
					self.motorStateSubGroup[10],
					self.motorStateSubGroup[11],
					self.motorStateSubGroup[12],
					self.motorStateSubGroup[13],
					self.motorStateSubGroup[14],
					self.motorStateSubGroup[15],
					self.motorStateSubGroup[16],
					self.motorStateSubGroup[17],
				},
                QLabel(""), strech = "0,1",
            }, }

    self.levelStateGroup = QGroupBox("Level State"){ layout = QVBoxLayout{
                QHBoxLayout{
                    self.levelStateSubGroup[1],
                    self.levelStateSubGroup[2],
                    self.levelStateSubGroup[3],
                    self.levelStateSubGroup[4],
                    self.levelStateSubGroup[5],
                    self.levelStateSubGroup[6],
                    self.levelStateSubGroup[7],
                    self.levelStateSubGroup[8],
                },
                QLabel(""), strech = "0,0,1",
            }, }

    self.layout = QVBoxLayout{
            self.motorStateGroup,
            self.levelStateGroup,
            QLabel(""), strech = "0,0,1",
        }
end

class "IotCtrlView"(QFrame)

function IotCtrlView:__init(number, socket)
    QFrame.__init(self)
    self.devRestartBtn = QPushButton("Restart")
    self.number = number
    self.socket = socket

    self.onMotor = {
        [1] = QCheckBox(self){checked=false},
    }
    self.offMotor = {
        [1] = QCheckBox(self){checked=false},
    }
    self.errMotor = {
        [1] = QCheckBox(self){checked=false},
    }
    self.onMotorSubGroup = {
        [1] = QGroupBox("all"){ 
            layout = QVBoxLayout{
                self.onMotor[1],
            },
        },
    }
    self.offMotorSubGroup = {
        [1] = QGroupBox("all"){ 
            layout = QVBoxLayout{
                self.offMotor[1],
            },
        },
    }
    self.errMotorSubGroup = {
        [1] = QGroupBox("all"){ 
            layout = QVBoxLayout{
                self.errMotor[1],
            },
        },
    }
    for i=2,17 do
        self.onMotor[#self.onMotor + 1] = QCheckBox(self){checked=false}
        self.offMotor[#self.offMotor + 1] = QCheckBox(self){checked=false}
        self.errMotor[#self.errMotor + 1] = QCheckBox(self){checked=false}
        if i <= (self.number+1) then
            self.onMotorSubGroup[#self.onMotorSubGroup + 1] = QGroupBox(""..(i-2)){ 
                layout = QVBoxLayout{ 
                    self.onMotor[i],
                },
            }
            self.offMotorSubGroup[#self.offMotorSubGroup + 1] = QGroupBox(""..(i-2)){ 
                layout = QVBoxLayout{ 
                    self.offMotor[i],
                },
            }
            self.errMotorSubGroup[#self.errMotorSubGroup + 1] = QGroupBox(""..(i-2)){ 
                layout = QVBoxLayout{ 
                    self.errMotor[i],
                },
            }
        else
            self.onMotorSubGroup[#self.onMotorSubGroup + 1] = QGroupBox(""..(i-2)){ 
                layout = QVBoxLayout{ 
                    self.onMotor[i],
                },
                hidden = true,
            }
            self.offMotorSubGroup[#self.offMotorSubGroup + 1] = QGroupBox(""..(i-2)){ 
                layout = QVBoxLayout{ 
                    self.offMotor[i],
                },
                hidden = true,
            }
            self.errMotorSubGroup[#self.errMotorSubGroup + 1] = QGroupBox(""..(i-2)){ 
                layout = QVBoxLayout{ 
                    self.errMotor[i],
                },
                hidden = true,
            }
        end
    end
    self.onMotorBtn = QPushButton("ON Motor", self) 
    self.offMotorBtn = QPushButton("OFF Motor", self)
    self.errMotorBtn = QPushButton("Clear Motor Err", self) 

    self.onMotorGroup = QGroupBox("Motor ON"){ layout = QVBoxLayout{
                QHBoxLayout{
                    self.onMotorSubGroup[1], 
                    self.onMotorSubGroup[2], 
                    self.onMotorSubGroup[3], 
                    self.onMotorSubGroup[4], 
                    self.onMotorSubGroup[5], 
                    self.onMotorSubGroup[6], 
                    self.onMotorSubGroup[7], 
                    self.onMotorSubGroup[8], 
                    self.onMotorSubGroup[9], 
                    self.onMotorSubGroup[10],
                    self.onMotorSubGroup[11],
                    self.onMotorSubGroup[12],
                    self.onMotorSubGroup[13],
                    self.onMotorSubGroup[14],
                    self.onMotorSubGroup[15],
                    self.onMotorSubGroup[16],
                    self.onMotorSubGroup[17],
                },
                self.onMotorBtn,
                QLabel(""), strech = "0,0,1",
            }, }

    self.offMotorGroup = QGroupBox("Motor OFF"){ layout = QVBoxLayout{
                QHBoxLayout{
                    self.offMotorSubGroup[1], 
                    self.offMotorSubGroup[2], 
                    self.offMotorSubGroup[3], 
                    self.offMotorSubGroup[4], 
                    self.offMotorSubGroup[5], 
                    self.offMotorSubGroup[6], 
                    self.offMotorSubGroup[7], 
                    self.offMotorSubGroup[8], 
                    self.offMotorSubGroup[9], 
                    self.offMotorSubGroup[10],
                    self.offMotorSubGroup[11],
                    self.offMotorSubGroup[12],
                    self.offMotorSubGroup[13],
                    self.offMotorSubGroup[14],
                    self.offMotorSubGroup[15],
                    self.offMotorSubGroup[16],
                    self.offMotorSubGroup[17],
                },
                self.offMotorBtn,
                QLabel(""), strech = "0,0,1",
            }, }
            
    self.clearMotorErrGroup = QGroupBox("Clear Motor Err"){ layout = QVBoxLayout{
                QHBoxLayout{
                    self.errMotorSubGroup[1], 
                    self.errMotorSubGroup[2], 
                    self.errMotorSubGroup[3], 
                    self.errMotorSubGroup[4], 
                    self.errMotorSubGroup[5], 
                    self.errMotorSubGroup[6], 
                    self.errMotorSubGroup[7], 
                    self.errMotorSubGroup[8], 
                    self.errMotorSubGroup[9], 
                    self.errMotorSubGroup[10],
                    self.errMotorSubGroup[11],
                    self.errMotorSubGroup[12],
                    self.errMotorSubGroup[13],
                    self.errMotorSubGroup[14],
                    self.errMotorSubGroup[15],
                    self.errMotorSubGroup[16],
                    self.errMotorSubGroup[17],
                },
                self.errMotorBtn,
                QLabel(""), strech = "0,0,1",
            }, }

    self.layout = QVBoxLayout{
            QHBoxLayout{
                self.devRestartBtn,
                QLabel(""), strech = "0,1",
            },

            self.onMotorGroup,

            self.offMotorGroup,

            self.clearMotorErrGroup,

            QLabel(""), strech = "0,0,0,0,1",
        }

    self.onMotor[1].clicked = function()
        for k=2,(self.number + 1) do
            self.onMotor[k].checked = self.onMotor[1].checked
        end
    end

    self.offMotor[1].clicked = function()
        for k=2,(self.number+1) do
            self.offMotor[k].checked = self.offMotor[1].checked
        end
    end  

    self.errMotor[1].clicked = function()
        for k=2,(self.number+1) do
            self.errMotor[k].checked = self.errMotor[1].checked
        end
    end    
end

class "IotRegView"(QFrame)

local readRegAddr = {
    0x00000000, 0x00000008, 0x0000000c, 0x00000010, 0x00000014, 0x01001000, 0x01001010, 0x01001020, 
    0x01001022, 0x00010000, 0x00010002, 0x00010004, 0x00010005, 0x00010006, 0x00010008, 0x00010046, 
    0x00010086
}
local readRegAddrString = {
    "ID", "Hard Ver", "Soft Ver", "Net Type", "RTC", "FTP User", "FTP Pwd", "FPT Port", 
    "FTP Host", "Motor State", "Motor Err", "Level High", "Level Low", "Motor 0 Current", "Motor 0 ERR Number", "Motor 0 time", 
    "Voice times"
}

local writeRegAddr = {
    0x01000014, 0x01001000, 0x01001010, 0x01001020, 0x01001022, 0x01010046, 0x01010086, 0x02010000, 
    0x03010000, 0x03010002
}
local writeRegAddrString = {
    "RTC", "FTP User", "FTP Pwd", "FPT Port", "FTP Host", "Motor 0 time", "Voice times", "Motor ON", 
    "Motor OFF", "Motor Err Clear"
}

function IotRegView:__init(number, socket)
    QFrame.__init(self)
    self.number = number
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

function IotInfoView:__init(number, socket)
    QFrame.__init(self)
    self.number = number
    self.socket = socket

    self.motorTimeSetAddr = {}
    self.motorString = {}
    for i=0,(number-1) do
        self.motorString[#self.motorString + 1] = "Motor "..i.." time"
        self.motorTimeSetAddr[#self.motorTimeSetAddr + 1] = 0x01010046 + i*4
    end

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
    self.selectMotorBox = QComboBox{self.motorString, editable = false}
    self.motorTimeEdit = QLineEdit{text = "", inputMask = "99999999999999"}
    self.motorTimeSetBtn = QPushButton("Set Motor Time")
    self.voiceTimesEdit = QLineEdit{text = "", inputMask = "99999"}
    self.voiceTimesSetBtn = QPushButton("Set Voice Times")
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
                QLabel("Set Motor Time"), self.selectMotorBox, self.motorTimeEdit, QLabel("s"), self.motorTimeSetBtn,
                QLabel(""), strech = "0,0,0,0,0,1",
            },
            QHBoxLayout{
                QLabel("Set Voice Times"), self.voiceTimesEdit, self.voiceTimesSetBtn,
                QLabel(""), strech = "0,0,0,1",
            },
            QLabel(""), strech = "0,0,0,0,0,0,0,1",
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
            self.maxMotors = QUtil.bitand((heartLen - 6)/4, 0xffffffff)
            -- 电机数最大为15
            if self.maxMotors > 15 then
                self.maxMotors = 15
            end
            logEdit:append("motor max numbers:"..self.maxMotors)

            if nil == self.info then
                self.info = IotInfoView(self.maxMotors, self.socket)
                self.tab:addTab(self.info, "Info view")
                self.tab:setCurrentWidget(self.info)

                self.reg = IotRegView(self.maxMotors, self.socket)
                self.tab:addTab(self.reg, "Reg view")
                self.tab:setCurrentWidget(self.reg)

                self.ctrl = IotCtrlView(self.maxMotors, self.socket)
                self.tab:addTab(self.ctrl, "Ctrl view")
                self.tab:setCurrentWidget(self.ctrl)

                self.state = IotStateView(self.maxMotors, self.socket)
                self.tab:addTab(self.state, "State view")
                self.tab:setCurrentWidget(self.state)
            end

            self.info.devSNText.text = sn
            self.info.devNetTypeText.text = netString
            self.info.hardVerText.text = string.format("%08x", hardVer)
            self.info.softVerVerText.text = string.format("%08x", softVer)
            self.info.heartAddrText.text = string.format("%08x", heartAddr)
            self.info.heartLenText.text = ""..heartLen
        end
    end

    self.parse_heart = function(data)
        if self.info then
            self.showData("heart data:", data)
            local len = data:byte(1) + data:byte(2) * 256
            local heartLen = tonumber(self.info.heartLenText.text)
            logEdit:append("recv len:"..len.." heart len:"..heartLen)
            local id=1
            if len == heartLen then
                local sn = ""
                for i=1,8 do
                    sn = sn..string.format("%02x ", data:byte(i))
                    id = id + 1
                end
                logEdit:append("recv sn:"..sn.."(register sn:"..self.info.devSNText.text..")")
                local heartSeq = data:byte(id) + data:byte(id + 1)*0x100 + data:byte(id + 2)*0x10000 + data:byte(id + 3)*0x1000000
                logEdit:append("heart sequence:"..string.format("%08x", heartSeq))
                id = id + 4
                local heartAddr = data:byte(id) + data:byte(id + 1)*0x100 + data:byte(id + 2)*0x10000 + data:byte(id + 3)*0x1000000
                logEdit:append("heart address:"..string.format("%08x", heartAddr))
                id = id + 4
                if self.info.devSNText.text:match(sn) else
                    local motorState = data:byte(id) + data:byte(id + 1)*0x100
                    logEdit:append("motor state:"..string.format("%04x", motorState))
                    id = id + 2
                    local motorErr = data:byte(id) + data:byte(id + 1)*0x100
                    logEdit:append("motor error:"..string.format("%04x", motorErr))
                    id = id + 2
                    local levelHigh = data:byte(id)
                    logEdit:append("high level:"..string.format("%04x", levelHigh))
                    id = id + 1
                    local levelLow = data:byte(id)
                    logEdit:append("low level:"..string.format("%04x", levelLow))
                    id = id + 1
                    local current = {}
                    for i=1,self.maxMotors do
                        current[#current + 1] = data:byte(id) + data:byte(id + 1)*0x100
                        id = id + 2
                    end

                    self.heartSeqText.text = ""..heartSeq
                        self.onMotor = {
        [1] = QCheckBox(self){checked=false},
    }
    self.offMotor = {
        [1] = QCheckBox(self){checked=false},
    }
    self.errMotor = {
        [1] = QCheckBox(self){checked=false},
    }
        self.motorState = {}
    self.motorErrNumber = {}
    self.motorCurrent = {}
    self.levelState = {}
                end
            end
        end
    end

    self.parse_request = function(data)
        if self.info then
            self.showData("request data:", data)
            local len = data:byte(1) + data:byte(2) * 256
        end
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
                if self.step == 1 then
                    if data[i] == STX then
                        self.step = self.step+1
                        self.bcc = 0
                        self.data = ""
                    end
                elseif self.step == 2 then
                    if data[i] == DLE then
                        self.step = self.step+1
                    elseif data[i] == ETX then
                        self.step = self.step+2
                    else
                        self.data = self.data..string.char(QUtil.bitand(data[i], 0xff))
                        self.bcc = QUtil.bitand(QUtil.bitxor(self.bcc, data[i]), 0xff)
                    end
                elseif self.step == 3 then
                    if data[i] == STX or data[i] == ETX or data[i] == DLE or data[i] == ACK or data[i] == NAK then
                        self.data = self.data..string.char(QUtil.bitand(data[i], 0xff))
                        self.bcc = QUtil.bitand(QUtil.bitxor(self.bcc, data[i]), 0xff)
                        self.step = self.step-1
                    else
                        self.step = 1
                    end
                elseif self.step == 4 then
                    if data[i] == self.bcc then
                        self.parse_data(self.data)
                        self.step = 1
                        if i < #data then
                            local tmpdata = {}
                            for k=i,#data do
                                tmpdata[#tmpdata + 1] = data[k]
                            end
                            self.server_client_view.readyRead(tmpdata)
                        end
                    else
                        self.step = 1
                    end
                end    
            end
        end
        --logEdit:append("Server socket ready read")
    end
end
