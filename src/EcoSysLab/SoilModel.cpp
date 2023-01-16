#include "SoilModel.hpp"

#include <cassert>
#include <iostream>

#include <glm/gtx/string_cast.hpp>

using namespace EcoSysLab;
using namespace std;
using namespace glm;


/* Coordinate system

The voxel position is its center.
Each voxel is dx wide.

				<-dx ->
				-------------------------
				|     |     |     |     |
				|  x  |  x  |  x  |  x  |
				|     |     |     |     |
				-------------------------
				   |     |     |     |
				   |     |     |     |
X-Coordinate:   -- 0 --- 1 --- 2 --- 3 -----

The 'm_volumePositionMin' stores the lower left corner of the lower left voxel.
I.e. for m_volumePositionMin=(0, 0) and m_resolution=(2, 2), and m_dx=1,
the voxel centers are at 0.5 and 1.5.

*/

void SoilModel::Initialize(const SoilParameters& p)
{
	assert(!m_initialized);

	m_diffusionForce = p.m_diffusionForce;
	m_gravityForce = p.m_gravityForce;
	m_nutrientForce = p.m_nutrientForce;
	m_dt = p.m_deltaTime;

	m_resolution = p.m_voxelResolution;
	m_dx = p.m_deltaX;
	m_boundingBoxMin = p.m_boundingBoxMin;

	m_boundary_x = p.m_boundary_x;
	m_boundary_y = p.m_boundary_y;
	m_boundary_z = p.m_boundary_z;

	m_blur_3x3_idx = vector<ivec3>({
		{-1, -1, -1},
		{ 0, -1, -1},
		{ 1, -1, -1},
				   
		{-1,  0, -1},
		{ 0,  0, -1},
		{ 1,  0, -1},
				   
		{-1,  1, -1},
		{ 0,  1, -1},
		{ 1,  1, -1},
				   
		{-1, -1,  0},
		{ 0, -1,  0},
		{ 1, -1,  0},
				   
		{-1,  0,  0},
		{ 0,  0,  0},
		{ 1,  0,  0},
				   
		{-1,  1,  0},
		{ 0,  1,  0},
		{ 1,  1,  0},
				   
		{-1, -1,  1},
		{ 0, -1,  1},
		{ 1, -1,  1},
				   
		{-1,  0,  1},
		{ 0,  0,  1},
		{ 1,  0,  1},
				   
		{-1,  1,  1},
		{ 0,  1,  1},
		{ 1,  1,  1},
		});

	m_blur_3x3_weights = vector<float>({
		0.009188900331780544,
		0.025493013475061985,
		0.009188900331780544,

		0.025493013475061978,
		0.0707259533321939,
		0.025493013475061978,

		0.009188900331780544,
		0.025493013475061985,
		0.009188900331780544,

		0.025493013475061978,
		0.0707259533321939,
		0.025493013475061978,

		0.0707259533321939,
		0.19621691565184837,
		0.0707259533321939,

		0.025493013475061978,
		0.0707259533321939,
		0.025493013475061978,

		0.009188900331780544,
		0.025493013475061985,
		0.009188900331780544,

		0.025493013475061978,
		0.0707259533321939,
		0.025493013475061978,

		0.009188900331780544,
		0.025493013475061985,
		0.009188900331780544
		});

	m_initialized = true;

	auto numVoxels = m_resolution.x * m_resolution.y * m_resolution.z;
	m_soilDensity = Field(numVoxels);
	for(int i = 0; i < m_soilDensity.size(); i++)
	{
		m_soilDensity[i] = p.m_soilDensitySampleFunc(GetPositionFromCoordinate(GetCoordinateFromIndex(i)));
	}

	m_rnd = std::mt19937(std::random_device()());

	Reset();

}

void SoilModel::Reset()
{
	assert(m_initialized);

	m_time = 0.f;

	auto numVoxels = m_resolution.x * m_resolution.y * m_resolution.z;
	auto empty = Field(numVoxels);
	empty = 0.f;

	// Water
	m_w = empty;

	// Capacity
	m_c = Field(numVoxels); // initialize with 1s
	m_c = 1.0f;

	//SetField(m_c, vec3(-10, -3, -10), vec3(10, 0, 10), 2);
	//BlurField(m_c);

	//ChangeField(m_c, vec3(0, 0, 0),   5000, 3);
	//ChangeField(m_c, vec3(-1, 0, -1), 5000, 3);
	//ChangeField(m_c, vec3(1, 0, -1),  5000, 3);
	//ChangeField(m_c, vec3(-1, 0, 1),  5000, 3);
	//ChangeField(m_c, vec3(1, 0, 1),   5000, 3);

	m_l = empty;


	// Nutrients
	m_n = empty;
	SetField(m_n, vec3(-10, -3, -10), vec3(10, 0, 10), 2);
	BlurField(m_n);
	// create some nutrients
	//ChangeNutrient(vec3(0,0,0),  20000, 4);
	//ChangeNutrient(vec3(1,3,0),   5000, 3);
	//ChangeNutrient(vec3(-5,-3,1), 10500, 5);


	// Permeability
	m_p = Field(numVoxels); // initialize with 1s
	m_p = 1.0f;
	//SetField(m_p, vec3(-6.4, -5, -6.4), vec3(6.4, 1, 6.4), 0.5f);
	SetField(m_p, vec3(-2, -5, -6.4), vec3(0, 1, 6.4), 0.0f);
	BlurField(m_p);
	//BlurField(m_p);


	// intermediate variables
	m_w_grad_x = empty;
	m_w_grad_y = empty;
	m_w_grad_z = empty;

	m_div_diff_x = empty;
	m_div_diff_y = empty;
	m_div_diff_z = empty;

	m_div_diff_n_x = empty;
	m_div_diff_n_y = empty;
	m_div_diff_n_z = empty;

	m_div_grav_x = empty;
	m_div_grav_y = empty;
	m_div_grav_z = empty;

	m_div_grav_n_x = empty;
	m_div_grav_n_y = empty;
	m_div_grav_n_z = empty;


	update_w_sum();

	m_version++;
}



void SoilModel::Convolution3(const Field& input, Field& output, const vector<int>& indices, const vector<float>& weights) const
{
	auto entries = m_resolution.x * m_resolution.y * m_resolution.z;
	assert(input.size()  == entries);
	assert(output.size() == entries);
	assert(indices.size() == weights.size());

	// for a 3D convolution:
	assert(m_resolution.x >= 3);
	assert(m_resolution.y >= 3);
	assert(m_resolution.z >= 3);

	// iterate over all indices that are not part of the boundary, where the whole convolution kernel can be applied
	for (auto x = 1; x < m_resolution.x - 1; ++x)
	{
		for (auto y = 1; y < m_resolution.y - 1; ++y)
		{
			for (auto z = 1; z < m_resolution.z - 1; ++z)
			{
				auto i = Index(x, y, z);
				output[i] = 0.f;
				for (auto j = 0; j < indices.size(); ++j)
				{
					output[i] += input[i + indices[j]] * weights[j];
				}
			}
		}
	}
}

void EcoSysLab::SoilModel::Boundary_Wrap_Axis(const Field& input, Field& output, const std::vector<int>& indices_1D, const std::vector<float>& weights, int lim_a, int lim_b, int lim_f, std::function<int(int, int, int)> WrapIndex) const
{
	for (int a = 0; a < lim_a; ++a)
	{
		for (int b = 0; b < lim_b; ++b)
		{
			output[WrapIndex(a, b, 0      )] = 0;
			output[WrapIndex(a, b, lim_f-1)] = 0;

			for(auto i=0u; i<indices_1D.size(); ++i)
			{
				auto idx = indices_1D[i];
				auto w   = weights[i];
				if( idx < 0 )
				{
					output[WrapIndex(a, b, 0      )] += input[WrapIndex(a, b, lim_f  +idx)] * w;
					output[WrapIndex(a, b, lim_f-1)] += input[WrapIndex(a, b, lim_f-1+idx)] * w;
				}
				else if( idx > 0 )
				{
					output[WrapIndex(a, b, 0      )] += input[WrapIndex(a, b, idx  )] * w;
					output[WrapIndex(a, b, lim_f-1)] += input[WrapIndex(a, b, idx-1)] * w;
				}
				else
				{
					output[WrapIndex(a, b, 0      )] += input[WrapIndex(a, b, 0      )] * w;
					output[WrapIndex(a, b, lim_f-1)] += input[WrapIndex(a, b, lim_f-1)] * w;
				}
			}
		}
	}
}


void EcoSysLab::SoilModel::Boundary_Wrap_X(const Field& input, Field& output, const std::vector<int>& indices_1D, const std::vector<float>& weights) const
{
	int lim_a = m_resolution.y;
	int lim_b = m_resolution.z;
	int lim_f = m_resolution.x;

	auto WrapIndex_X = [&](int a, int b, int f)
	{
		return Index(f, a, b);
	};

	Boundary_Wrap_Axis(input, output, indices_1D, weights, lim_a, lim_b, lim_f, WrapIndex_X);
}

void EcoSysLab::SoilModel::Boundary_Wrap_Y(const Field& input, Field& output, const std::vector<int>& indices_1D, const std::vector<float>& weights) const
{
	int lim_a = m_resolution.x;
	int lim_b = m_resolution.z;
	int lim_f = m_resolution.y;

	auto WrapIndex_X = [&](int a, int b, int f)
	{
		return Index(a, f, b);
	};

	Boundary_Wrap_Axis(input, output, indices_1D, weights, lim_a, lim_b, lim_f, WrapIndex_X);
}

void EcoSysLab::SoilModel::Boundary_Wrap_Z(const Field& input, Field& output, const std::vector<int>& indices_1D, const std::vector<float>& weights) const
{
	int lim_a = m_resolution.x;
	int lim_b = m_resolution.y;
	int lim_f = m_resolution.z;

	auto WrapIndex_X = [&](int a, int b, int f)
	{
		return Index(a, b, f);
	};

	Boundary_Wrap_Axis(input, output, indices_1D, weights, lim_a, lim_b, lim_f, WrapIndex_X);
}


bool SoilModel::Initialized() const
{
	return m_initialized;
}

float SoilModel::GetTime() const
{
	return m_time;
}

void EcoSysLab::SoilModel::update_w_sum()
{
	// count total water:
	m_w_sum = 0.f;
	for (auto i = 0; i < m_w.size(); ++i)
		m_w_sum += m_w[i];
}


float AbsorptionValueGaussian(int region_width, int border_distance)
{
	auto a = exp(- ((float)border_distance*(float)border_distance) / (0.2*(float)region_width*(float)region_width));
	return 1-a;
}


void SoilModel::Step()
{
	assert(m_initialized);

	const auto num_voxels = m_w.size();
	Field tmp(num_voxels);

	const auto grad_index_x = vector<int>({
		Index(-1, 0, 0),
		Index(+1, 0, 0),
		});
	const auto grad_index_y = vector<int>({
		Index(0, -1, 0),
		Index(0, +1, 0),
		});
	const auto grad_index_z = vector<int>({
		Index(0, 0, -1),
		Index(0, 0, +1),
		});
	const auto grad_index_1D = vector<int>({-1, 1});

	// ----------------- diffusion -----------------
	{
		if(m_use_capacity)
		{
			for(auto i=0; i<num_voxels; ++i)
				m_l[i] = m_w[i] / m_c[i];
		}
		else
			m_l = m_w;


		const auto wx_d = 1.0f / (2.0f * m_dx);
		const auto grad_weights = vector<float>({ -wx_d, wx_d });

		// compute gradient dw
		Convolution3(m_l, m_w_grad_x, grad_index_x, grad_weights);
		Convolution3(m_l, m_w_grad_y, grad_index_y, grad_weights);
		Convolution3(m_l, m_w_grad_z, grad_index_z, grad_weights);

		if(Boundary::wrap == m_boundary_x)
			Boundary_Wrap_X(m_l, m_w_grad_x, grad_index_1D, grad_weights);
		if(Boundary::wrap == m_boundary_y)
			Boundary_Wrap_Y(m_l, m_w_grad_y, grad_index_1D, grad_weights);
		if(Boundary::wrap == m_boundary_z)
			Boundary_Wrap_Z(m_l, m_w_grad_z, grad_index_1D, grad_weights);

		// apply effect of permeability
		// it must be applied after computing the gradient, since it is inhomogeneous!
		m_w_grad_x *= m_p;
		m_w_grad_y *= m_p;
		m_w_grad_z *= m_p;


		// compute divergence
		Convolution3(m_w_grad_x, m_div_diff_x, grad_index_x, grad_weights);
		Convolution3(m_w_grad_y, m_div_diff_y, grad_index_y, grad_weights);
		Convolution3(m_w_grad_z, m_div_diff_z, grad_index_z, grad_weights);

		if(Boundary::wrap == m_boundary_x)
			Boundary_Wrap_X(m_w_grad_x, m_div_diff_x, grad_index_1D, grad_weights);
		if(Boundary::wrap == m_boundary_y)
			Boundary_Wrap_Y(m_w_grad_y, m_div_diff_y, grad_index_1D, grad_weights);
		if(Boundary::wrap == m_boundary_z)
			Boundary_Wrap_Y(m_w_grad_z, m_div_diff_z, grad_index_1D, grad_weights);

		// divergence for nutrients
		tmp = m_w_grad_x * m_diffusionForce * m_n;
		Convolution3(tmp, m_div_diff_n_x, grad_index_x, grad_weights);
		if(Boundary::wrap == m_boundary_x)
			Boundary_Wrap_X(tmp, m_div_diff_n_x, grad_index_1D, grad_weights);

		tmp = m_w_grad_y * m_diffusionForce * m_n;
		Convolution3(tmp, m_div_diff_n_y, grad_index_y, grad_weights);
		if(Boundary::wrap == m_boundary_y)
			Boundary_Wrap_X(tmp, m_div_diff_n_y, grad_index_1D, grad_weights);
		
		tmp = m_w_grad_z * m_diffusionForce * m_n;
		Convolution3(tmp, m_div_diff_n_z, grad_index_z, grad_weights);
		if(Boundary::wrap == m_boundary_z)
			Boundary_Wrap_X(tmp, m_div_diff_n_z, grad_index_1D, grad_weights);

		m_div_diff_x *= m_diffusionForce;
		m_div_diff_y *= m_diffusionForce;
		m_div_diff_z *= m_diffusionForce;
	}



	// ------------ gravity ------------
	
	auto wp  = m_w * m_p;
	auto wpn =  wp * m_n;

	// TODO: the weights are computed from the gravity force. however this is inhomogeneously altered by the permeability.
	// A better integration scheme is required that accounts for this and is still stable.

	// X direction:
	{
		auto a_x = m_gravityForce.x;
		auto wx = a_x * 1.f/(2.f*m_dx);
		auto theta = (a_x * m_dt/m_dx) * (a_x * m_dt/m_dx);
		auto wt = theta * 1/(2*m_dt);

		const auto idx = vector<int>({
			Index( 1, 0, 0),
			Index(-1, 0, 0),
			Index( 1, 0, 0),
			Index( 0, 0, 0),
			Index(-1, 0, 0),
			});
		const auto idx_1D = vector<int>({1, -1, 1, 0, -1});

		const auto weights = vector<float>({
			-wx, wx, wt, -2*wt, wt
			});

		Convolution3(wp, m_div_grav_x, idx, weights);
		if( Boundary::wrap == m_boundary_x )
			Boundary_Wrap_X(wp, m_div_grav_x, idx_1D, weights);

		// gravity force on nutrients
		Convolution3(wpn, m_div_grav_n_x, idx, weights);
		if( Boundary::wrap == m_boundary_x )
			Boundary_Wrap_X(wpn, m_div_grav_n_x, idx_1D, weights);
	}

	// Y direction:
	{
		auto a_y = m_gravityForce.y;
		auto wx = a_y * 1.f/(2.f*m_dx);
		auto theta = (a_y * m_dt/m_dx) * (a_y * m_dt/m_dx);
		auto wt = theta * 1/(2*m_dt);

		const auto idx = vector<int>({
			Index(0,  1, 0),
			Index(0, -1, 0),
			Index(0,  1, 0),
			Index(0,  0, 0),
			Index(0, -1, 0),
			});
		const auto idx_1D = vector<int>({1, -1, 1, 0, -1});

		const auto weights = vector<float>({
			-wx, wx, wt, -2*wt, wt
			});

		Convolution3(wp, m_div_grav_y, idx, weights);
		if( Boundary::wrap == m_boundary_y )
			Boundary_Wrap_X(wp, m_div_grav_y, idx_1D, weights);

		// gravity force on nutrients
		Convolution3(wpn, m_div_grav_n_y, idx, weights);
		if( Boundary::wrap == m_boundary_y )
			Boundary_Wrap_X(wpn, m_div_grav_n_y, idx_1D, weights);
	}

	// Z direction:
	{
		auto a_z = m_gravityForce.z;
		auto wx = a_z * 1.f/(2.f*m_dx);
		auto theta = (a_z * m_dt/m_dx) * (a_z * m_dt/m_dx);
		auto wt = theta * 1/(2*m_dt);

		const auto idx = vector<int>({
			Index(0, 0,  1),
			Index(0, 0, -1),
			Index(0, 0,  1),
			Index(0, 0,  0),
			Index(0, 0, -1),
			});
		const auto idx_1D = vector<int>({1, -1, 1, 0, -1});

		const auto weights = vector<float>({
			-wx, wx, wt, -2*wt, wt
			});

		Convolution3(wp, m_div_grav_z, idx, weights);
		if( Boundary::wrap == m_boundary_z )
			Boundary_Wrap_X(wp, m_div_grav_z, idx_1D, weights);

		// gravity force on nutrients
		Convolution3(wpn, m_div_grav_n_z, idx, weights);
		if( Boundary::wrap == m_boundary_z )
			Boundary_Wrap_X(wpn, m_div_grav_n_z, idx_1D, weights);
	}

	// apply all the fluxes:
	for (auto i = 0; i < num_voxels; ++i)
	{
		auto divergence = (m_div_diff_x[i] + m_div_diff_y[i] + m_div_diff_z[i])
			            + (m_div_grav_x[i] + m_div_grav_y[i] + m_div_grav_z[i]);
		// ToDo: Also apply source terms here
		m_w[i] += m_dt * divergence;

		// update nutrients:
		auto divergence_nut = (m_div_diff_n_x[i] + m_div_diff_n_y[i] + m_div_diff_n_z[i])
			                + (m_div_grav_n_x[i] + m_div_grav_n_y[i] + m_div_grav_n_z[i]);
		m_n[i] += m_dt * divergence_nut * m_nutrientForce;
	}



	// absorbing boundary regions
	int region_width = 5;

	if( Boundary::absorb == m_boundary_x )
	{
		for(auto i=0; i<region_width; ++i)
		{
			auto a = AbsorptionValueGaussian(region_width, i);
			for (auto y = 0u; y < m_resolution.y; ++y)
			{
				for (auto z = 0u; z < m_resolution.z; ++z)
				{
					m_w[Index(i,                  y, z)] *= a;
					m_w[Index(m_resolution.x-1-i, y, z)] *= a;
				}
			}
		}
	}

	if( Boundary::absorb == m_boundary_y )
	{
		for(auto i=0; i<region_width; ++i)
		{
			auto a = AbsorptionValueGaussian(region_width, i);
			for (auto x = 0u; x < m_resolution.x; ++x)
			{
				for (auto z = 0u; z < m_resolution.z; ++z)
				{
					m_w[Index(x, i,                  z)] *= a;
					m_w[Index(x, m_resolution.y-1-i, z)] *= a;
				}
			}
		}
	}

	if( Boundary::absorb == m_boundary_z )
	{
		for(auto i=0; i<region_width; ++i)
		{
			auto a = AbsorptionValueGaussian(region_width, i);
			for(auto x = 0u; x<m_resolution.x; ++x)
			{
				for (auto y = 0u; y < m_resolution.y; ++y)
				{
					m_w[Index(x, y, i                 )] *= a;
					m_w[Index(x, y, m_resolution.z-1-i)] *= a;
				}
			}
		}
	}

	m_time += m_dt;

	update_w_sum();

	m_version++;
}

void EcoSysLab::SoilModel::Irrigation()
{
	m_rnd = std::mt19937(27);

	auto bb_min = GetBoundingBoxMin();
	auto bb_max = GetBoundingBoxMax();
	std::uniform_real_distribution<> dist_x(bb_min.x, bb_max.x);
	std::uniform_real_distribution<> dist_y(bb_min.y, bb_max.y);
	std::uniform_real_distribution<> dist_z(bb_min.z, bb_max.z);

	std::uniform_real_distribution<> width(0.5, 2);

	/*
	for(auto i=0; i<30; ++i)
	{
		auto pos = vec3(dist_x(m_rnd), dist_y(m_rnd), dist_z(m_rnd));
		auto amount = m_irrigationAmount * GetDensity(pos);
		ChangeWater(pos, amount, width(m_rnd));
	}*/
	ChangeWater(vec3(0, 2, 0), m_irrigationAmount, 1);

	/*
	if ((int)(m_time / 20.0) % 2 == 0)
	{
		ChangeWater(vec3(-18, 0, 15), 10, 8);
	}
	if ((int)((m_time + 5) / 19.0) % 2 == 0)
	{
		ChangeWater(vec3(0, 20, -15), -10, 15);
	}*/
}



float SoilModel::GetWater(const vec3& position) const
{
	return GetField(m_w, position, 0.0f);
}

float SoilModel::GetDensity(const vec3& position) const
{
	return GetField(m_soilDensity, position, 1000.0f);
}

float SoilModel::GetNutrient(const vec3& position) const
{
	return GetField(m_n, position, 0.0f);
}

float EcoSysLab::SoilModel::GetCapacity(const glm::vec3& position) const
{
	return GetField(m_c, position, 1.0f);
}

float EcoSysLab::SoilModel::GetField(const Field& field, const glm::vec3& position, float default_value) const
{
	if( ! PositionInsideVolume(position) )
		return default_value;
	return field[Index(GetCoordinateFromPosition(position))];
}

void SoilModel::ChangeField(Field& field, const vec3& center, float amount, float width)
{
	width /= 3.0; // seems ok :D
	auto cutoff = 3.0; // how much of the gaussian to keep

	auto voxel_min = GetCoordinateFromPosition(center - vec3(width * cutoff));
	auto voxel_max = GetCoordinateFromPosition(center + vec3(width * cutoff)) + ivec3(2);

	voxel_min = glm::max(voxel_min, ivec3(0));
	voxel_max = glm::min(voxel_max, static_cast<ivec3>(m_resolution)-ivec3(1));

	// the <= is important here
	float sum = 0.f;
	for (auto z=voxel_min.z; z <= voxel_max.z; ++z)
	{
		for (auto y=voxel_min.y; y <= voxel_max.y; ++y)
		{
			for (auto x=voxel_min.x; x <= voxel_max.x; ++x)
			{
				auto pos = GetPositionFromCoordinate({ x, y, z });
				auto l = glm::length(pos - center);
				auto v = glm::exp( - l*l / (2* width*width));
				sum += v;
			}
		}
	}

	for (auto z = voxel_min.z; z <= voxel_max.z; ++z)
	{
		for (auto y = voxel_min.y; y <= voxel_max.y; ++y)
		{
			for (auto x = voxel_min.x; x <= voxel_max.x; ++x)
			{
				auto pos = GetPositionFromCoordinate({ x, y, z });
				auto l = glm::length(pos - center);
				auto v = glm::exp( - l*l / (2* width*width));
				field[Index(x, y, z)] += v / sum * amount;
			}
		}
	}

}

void EcoSysLab::SoilModel::SetField(Field& field, const vec3& bb_min, const vec3& bb_max, float value)
{
	auto idx_min = GetCoordinateFromPosition(bb_min);
	auto idx_max = GetCoordinateFromPosition(bb_max);
	idx_min = glm::clamp(idx_min, ivec3(0, 0, 0), m_resolution);
	idx_max = glm::clamp(idx_max, ivec3(0, 0, 0), m_resolution);

	for(int z=idx_min.z; z<idx_max.z; ++z)
	{
		for(int y=idx_min.y; y<idx_max.y; ++y)
		{
			for(int x=idx_min.x; x<idx_max.x; ++x)
			{
				field[Index(x, y, z)] = value;
			}
		}
	}
}


void EcoSysLab::SoilModel::BlurField(Field& field)
{
	// this will ignore corners and introduce artifacts
	/*
	Convolution3(field, tmp, m_blur_3x3_idx, m_blur_3x3_weights);
	Convolution3(tmp, field, m_blur_3x3_idx, m_blur_3x3_weights);
	*/

	Field tmp(field.size());

	for(int z=0; z<m_resolution.z; ++z)
	{
		for(int y=0; y<m_resolution.y; ++y)
		{
			for(int x=0; x<m_resolution.x; ++x)
			{

				// iterate over the blur kernel, ignore pixels that are out of the field.
				float total_weight = 0.0f;
				tmp[Index(x, y, z)] = 0.0f;

				for(int i = 0; i<m_blur_3x3_idx.size(); ++i)
				{
					ivec3 idx = ivec3(x, y, z) + m_blur_3x3_idx[i];
					// method 1: Ignore outlier
					/*
					{
						if(    idx.x>=0 && idx.x<m_resolution.x
							&& idx.y>=0 && idx.y<m_resolution.y
							&& idx.z>=0 && idx.z<m_resolution.z)
						{
							total_weight += m_blur_3x3_weights[i];
							tmp[Index(x, y, z)] += field[Index(idx)] * m_blur_3x3_weights[i];
						}
					}*/

					//Method 2: clamp outlier
					{
						//cout << "before " << to_string(idx) << endl;
						idx = glm::clamp(idx, ivec3(0, 0, 0), m_resolution-ivec3(1));
						//cout << "after  " << to_string(idx) << endl;
						total_weight += m_blur_3x3_weights[i];
						tmp[Index(x, y, z)] += field[Index(idx)] * m_blur_3x3_weights[i];
					}

				}
				tmp[Index(x, y, z)] /= total_weight;
			}
		}
	}
	field = tmp;
}

void SoilModel::ChangeWater(const vec3& center, float amount, float width)
{
	ChangeField(m_w, center, amount, width);
	update_w_sum();
}

void SoilModel::ChangeDensity(const vec3& center, float amount, float width)
{
	ChangeField(m_soilDensity, center, amount, width);
}

void SoilModel::ChangeNutrient(const vec3& center, float amount, float width)
{
	ChangeField(m_n, center, amount, width);
}

void EcoSysLab::SoilModel::ChangeCapacity(const glm::vec3& center, float amount, float width)
{
	ChangeField(m_c, center, amount, width);
}


int EcoSysLab::SoilModel::Index(const ivec3& resolution, int x, int y, int z)
{
	return x + y * resolution.x + z * resolution.x * resolution.y;
}

int SoilModel::Index(const int x, const int y, const int z) const
{
	return Index(m_resolution, x, y, z);
}

int EcoSysLab::SoilModel::Index(const ivec3& resolution, const ivec3& c)
{
	return Index(resolution, c.x, c.y, c.z);
}

int SoilModel::Index(const ivec3& c) const
{
	return Index(m_resolution, c.x, c.y, c.z);
}



ivec3 SoilModel::GetCoordinateFromIndex(const int index) const
{
	return {
		index %  m_resolution.x,
		index % (m_resolution.x * m_resolution.y) / m_resolution.x,
		index / (m_resolution.x * m_resolution.y) };
}

ivec3 SoilModel::GetCoordinateFromPosition(const vec3& pos) const
{
	//return {
	//	floor((pos.x - (m_boundingBoxMin.x + m_dx/2.0)) / m_dx),
	//	floor((pos.y - (m_boundingBoxMin.y + m_dx/2.0)) / m_dx),
	//	floor((pos.z - (m_boundingBoxMin.z + m_dx/2.0)) / m_dx)
	//};
	return {
		floor((pos.x - m_boundingBoxMin.x) / m_dx),
		floor((pos.y - m_boundingBoxMin.y) / m_dx),
		floor((pos.z - m_boundingBoxMin.z) / m_dx)
	};
}

vec3 SoilModel::GetPositionFromCoordinate(const ivec3& coordinate) const
{
	return {
		m_boundingBoxMin.x + (m_dx/2.0) + coordinate.x * m_dx,
		m_boundingBoxMin.y + (m_dx/2.0) + coordinate.y * m_dx,
		m_boundingBoxMin.z + (m_dx/2.0) + coordinate.z * m_dx
	};
}


ivec3 SoilModel::GetVoxelResolution() const
{
	return m_resolution;
}

float SoilModel::GetVoxelSize() const
{
	return m_dx;
}

vec3 SoilModel::GetBoundingBoxMin() const
{
	return m_boundingBoxMin;
}

vec3 EcoSysLab::SoilModel::GetBoundingBoxMax() const
{
	return m_boundingBoxMin + vec3(m_resolution)*m_dx;
}

bool EcoSysLab::SoilModel::PositionInsideVolume(const glm::vec3& p) const
{
	auto min = GetBoundingBoxMin();
	auto max = GetBoundingBoxMax();
	if ( p.x < min.x || p.y < min.y || p.z < min.z )
		return false;
	if ( p.x >= max.x || p.y >= max.y || p.z >= max.z )
		return false;
	return true;
}
