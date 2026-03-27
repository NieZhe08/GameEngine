ParticleController = {
	
	OnStart = function(self)
		math.randomseed(498)

		for i = 0,25 do
			local big_system = Actor.Instantiate("big_particle_system_actor")
			local ps = big_system:GetComponent("ParticleSystem")
			ps.x = i * 3
			ps.start_color_r = math.random(0, 255)
			ps.start_color_g = math.random(0, 255)
			ps.start_color_b = math.random(0, 255)
		end
	end
}