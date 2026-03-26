CameraController = {
	x_pos = 0,
	y_pos = 0,
	
	OnUpdate = function(self)
		if Input.GetKey("right") then
			self.x_pos = self.x_pos + 0.1
		end
		
		if Input.GetKey("left") then
			self.x_pos = self.x_pos - 0.1
		end
		
		if Input.GetKey("up") then
			self.y_pos = self.y_pos - 0.1
		end
		
		if Input.GetKey("down") then
			self.y_pos = self.y_pos + 0.1
		end
		
		Camera.SetPosition(self.x_pos, self.y_pos)
	end
}