ParticleController = {
	particle_actor = nil,

	OnUpdate = function(self)
		if Application.GetFrame() == 10 then
			Debug.Log("spawning particle actor")
			self.particle_actor = Actor.Instantiate("particle_actor")
		
		elseif Application.GetFrame() == 120 then
			Debug.Log("destroying particle actor")

			Actor.Destroy(self.particle_actor)

		end
	end
}

