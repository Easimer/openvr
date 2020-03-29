TrackedDeviceServerDriver = {}
VRLeftEye	= 0
VRRightEye	= 1

TrackingResult_Uninitialized			= 1
TrackingResult_Calibrating_InProgress	= 100
TrackingResult_Calibrating_OutOfRange	= 101
TrackingResults_Running_OK				= 200
TrackingResults_Running_OutOfRange		= 201
TrackingResult_Fallback_RotationOnly	= 300

Quat = {}
Quat.mt = {}
Quat.mt.__mul = function(lhs, rhs)
	local w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z
	local x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y
	local y = lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z
	local z = lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x

	return Quat.new(w, x, y, z)
end

function Quat.identity()
	return Quat.new(1, 0, 0, 0)
end

function Quat.new(w, x, y, z)
	local ret = {}
	ret.w = w
	ret.x = x
	ret.y = y
	ret.z = z
	setmetatable(ret, Quat.mt)
	return ret
end

function Quat:conjugate()
	return Quat.new(self.w, -self.x, -self.y, -self.z)
end

function Quat:inverse()
	local qa = self.conjugate()
	local len = self.w * self.w + self.x * self.x + self.y * self.y + self.z * self.z
	qa.w = qa.w / len
	qa.x = qa.x / len
	qa.y = qa.y / len
	qa.z = qa.z / len
	return qa
end

function TrackedDeviceServerDriver:OnInit()
	DriverLog("Initializing HMD!");
	self.nRenderWidth = 800
	self.nRenderHeight = 900
	self.nWindowWidth = 1600
	self.nWindowHeight = 900
	self.nWindowX = 100
	self.nWindowY = 100
end

function TrackedDeviceServerDriver:OnShutdown()
	DriverLog("Cleaning up HMD!");
end

-- Returns a tuple of (WindowX, WindowY, WindowW, WindowH)
function TrackedDeviceServerDriver:GetWindowBounds()
	return self.nWindowX, self.nWindowY, self.nWindowWidth, self.nWindowHeight
end

function TrackedDeviceServerDriver:Activate(unObjectId)
	self.unObjectId = unObjectId
	DriverLog("HMD activation objectId=" .. self.unObjectId)
	return true
end

function TrackedDeviceServerDriver:EnterStandby()
	DriverLog("Entering standby")
end

function TrackedDeviceServerDriver:RunFrame()
end

-- Returns a tuple of (RenderW, RenderH)
function TrackedDeviceServerDriver:GetRecommendedRenderTargetSize()
	return self.nRenderWidth, self.nRenderHeight
end

-- Returns a tuple of (WindowX, WindowY, WindowW, WindowH)
function TrackedDeviceServerDriver:GetEyeOutputViewport(eye)
	local y = 0
	local width = self.nWindowWidth / 2
	local height = self.nWindowHeight
	local x = 0

	if eye == VRLeftEye then
		x = 0
	else
		x = self.nWindowWidth / 2
	end

	return x, y, width, height
end

function TrackedDeviceServerDriver:GetPose()
	local ret = {}
	ret.poseIsValid = true
	ret.result = TrackingResult_Calibrating_InProgress
	ret.deviceIsConnected = true

	ret.qWorldFromDriverRotation = Quat.identity()
	ret.qDriverFromHeadRotation = Quat.identity()
	ret.qRotation = Quat.identity()
	ret.vecWorldFromDriverTranslation = {0, 0, 0}

	return ret
end
