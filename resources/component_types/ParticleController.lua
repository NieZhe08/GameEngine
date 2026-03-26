ParticleController = {
	ps = nil,

	OnStart = function(self)
		local particle_actor = Actor.Instantiate("particle_actor")
		self.ps = particle_actor:GetComponent("ParticleSystem")
		self.ps:Stop()
	end,

	OnUpdate = function(self)
		if Input.GetKeyDown("space") then

			Debug.Log("Frame " .. Application.GetFrame() .. ": Burst!")
			self.ps:Burst()

		end
	end
}

