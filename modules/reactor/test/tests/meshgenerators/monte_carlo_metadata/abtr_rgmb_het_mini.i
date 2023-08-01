# TODO Define global parameters here
# ==============================================================================
# Geometry Info.
# ==============================================================================
fuel_pin_pitch = 0.908
fuel_clad_r_i = 0.348
fuel_clad_r_o = 0.4
control_pin_pitch = 1.243
control_clad_r_i = 0.485
control_clad_r_o = 0.555
control_duct_pitch_inner = 12.198
control_duct_pitch_outer = 12.798
duct_pitch_inner = 13.598
duct_pitch_outer = 14.198
assembly_pitch = 14.598

z_active_core_lower = 60
z_active_core_upper = 140
z_sodium_gp_upper = 160
z_gp_upper = 260

dz_active_core_lower = ${fparse z_active_core_lower - 0}
dz_active_core_upper = ${fparse z_active_core_upper - z_active_core_lower}
dz_sodium_gp_upper = ${fparse z_sodium_gp_upper - z_active_core_upper}
dz_gp_upper = ${fparse z_gp_upper - z_sodium_gp_upper}

max_axial_mesh_size = 20
naxial_active_core_lower = ${fparse dz_active_core_lower / max_axial_mesh_size}
naxial_active_core_upper = ${fparse dz_active_core_upper / max_axial_mesh_size}
naxial_sodium_gp_upper = ${fparse dz_sodium_gp_upper / max_axial_mesh_size}
naxial_gp_upper = ${fparse dz_gp_upper / max_axial_mesh_size}

# ==============================================================================
# Material IDs
# ==============================================================================
mid_fuel_1 = 1
# mid_fuel_2 = 2
# mid_fuel_3 = 3
mid_b4c = 4
mid_ht9 = 5
mid_sodium = 6
mid_rad_refl = 7
# mid_rad_shld = 8
mid_lower_refl = 9
mid_upper_na_plen = 10
mid_upper_gas_plen = 11
mid_control_empty = 12

# ==============================================================================
# Mesh
# ==============================================================================
[Mesh]
  # Define global reactor mesh parameters
  [rmp]
    type = ReactorMeshParams
    dim = 3
    geom = "Hex"
    assembly_pitch = ${assembly_pitch}
    axial_regions  = '${dz_active_core_lower} ${dz_active_core_upper} ${dz_sodium_gp_upper} ${dz_gp_upper}'
    axial_mesh_intervals = '${naxial_active_core_lower} ${naxial_active_core_upper}
                            ${naxial_sodium_gp_upper} ${naxial_gp_upper}'
    top_boundary_id = 201
    bottom_boundary_id = 202
    radial_boundary_id = 203
    #generate_rgmb_metadata = true
    mc_geometry = core
  []

  # Define constituent pins of fuel assemblies
  [fuel_pin_1]
    type = PinMeshGenerator
    reactor_params = rmp
    pin_type = 1
    pitch = ${fuel_pin_pitch}
    num_sectors = 2
    ring_radii = '${fuel_clad_r_i} ${fuel_clad_r_o}'
    mesh_intervals = '1 1 1'    # Fuel, cladding, background
    region_ids = '${mid_lower_refl}     ${mid_lower_refl}     ${mid_lower_refl};
                  ${mid_fuel_1}         ${mid_ht9}            ${mid_sodium};
                  ${mid_upper_na_plen}  ${mid_upper_na_plen}  ${mid_upper_na_plen};
                  ${mid_upper_gas_plen} ${mid_upper_gas_plen} ${mid_upper_gas_plen}' # Fuel, cladding, background
    quad_center_elements = false
    show_rgmb_metadata = true
  []
  # Define fuel assemblies
  [fuel_assembly_1]
    type = AssemblyMeshGenerator
    assembly_type = 1
    background_intervals = 1
    background_region_id = '${mid_lower_refl} ${mid_sodium} ${mid_upper_na_plen} ${mid_upper_gas_plen}'
    duct_halfpitch = '${fparse duct_pitch_inner / 2} ${fparse duct_pitch_outer / 2}'
    duct_intervals = '1 1'
    duct_region_ids = ' ${mid_lower_refl}     ${mid_lower_refl};
                        ${mid_ht9}            ${mid_sodium};
                        ${mid_upper_na_plen}  ${mid_upper_na_plen};
                        ${mid_upper_gas_plen} ${mid_upper_gas_plen}'
    inputs = 'fuel_pin_1'
    pattern = '0 0 0 0 0 0 0 0 0;
              0 0 0 0 0 0 0 0 0 0;
             0 0 0 0 0 0 0 0 0 0 0;
            0 0 0 0 0 0 0 0 0 0 0 0;
           0 0 0 0 0 0 0 0 0 0 0 0 0;
          0 0 0 0 0 0 0 0 0 0 0 0 0 0;
         0 0 0 0 0 0 0 0 0 0 0 0 0 0 0;
        0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0;
       0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0;
        0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0;
         0 0 0 0 0 0 0 0 0 0 0 0 0 0 0;
          0 0 0 0 0 0 0 0 0 0 0 0 0 0;
           0 0 0 0 0 0 0 0 0 0 0 0 0;
            0 0 0 0 0 0 0 0 0 0 0 0;
             0 0 0 0 0 0 0 0 0 0 0;
              0 0 0 0 0 0 0 0 0 0;
               0 0 0 0 0 0 0 0 0'
    show_rgmb_metadata = true
  []

  # Define constituent pin of control assembly
  [control_pin]
    type = PinMeshGenerator
    reactor_params = rmp
    pin_type = 4
    pitch = ${control_pin_pitch}
    num_sectors = 2
    ring_radii = '${control_clad_r_i} ${control_clad_r_o}'
    mesh_intervals = '1 1 1'    # Fuel, cladding, background
    region_ids = '${mid_control_empty} ${mid_control_empty} ${mid_control_empty};
                  ${mid_control_empty} ${mid_control_empty} ${mid_control_empty};
                  ${mid_b4c}            ${mid_ht9}            ${mid_sodium};
                  ${mid_b4c}            ${mid_ht9}            ${mid_sodium};'     # Fuel, cladding, background
    quad_center_elements = false
    show_rgmb_metadata = true
  []
  # Define control assembly
  [control_assembly]
    type = AssemblyMeshGenerator
    assembly_type = 4
    background_intervals = 1
    background_region_id = '${mid_control_empty} ${mid_control_empty} ${mid_sodium} ${mid_sodium}'
    duct_halfpitch = '${fparse control_duct_pitch_inner / 2} ${fparse control_duct_pitch_outer / 2}
                      ${fparse duct_pitch_inner / 2}         ${fparse duct_pitch_outer / 2}'
    duct_intervals = '1 1 1 1'
    duct_region_ids = '${mid_control_empty} ${mid_control_empty} ${mid_control_empty} ${mid_control_empty};
                       ${mid_control_empty} ${mid_control_empty} ${mid_control_empty} ${mid_control_empty};
                       ${mid_ht9}           ${mid_sodium}        ${mid_ht9}           ${mid_sodium};
                       ${mid_ht9}           ${mid_sodium}        ${mid_ht9}           ${mid_sodium}'
    inputs = 'control_pin'
    pattern = '0 0 0 0 0 0;
              0 0 0 0 0 0 0;
             0 0 0 0 0 0 0 0;
            0 0 0 0 0 0 0 0 0;
           0 0 0 0 0 0 0 0 0 0;
          0 0 0 0 0 0 0 0 0 0 0;
           0 0 0 0 0 0 0 0 0 0;
            0 0 0 0 0 0 0 0 0;
             0 0 0 0 0 0 0 0;
              0 0 0 0 0 0 0;
               0 0 0 0 0 0'
    show_rgmb_metadata = true
  []

  # Define homogenized reflector and shielding assemblies
  [reflector_assembly]
    type = PinMeshGenerator
    reactor_params = rmp
    pin_type = 5
    pitch = ${assembly_pitch}
    num_sectors = 2
    mesh_intervals = '1'
    region_ids = '${mid_rad_refl};
                  ${mid_rad_refl};
                  ${mid_rad_refl};
                  ${mid_rad_refl}' # Background
    use_as_assembly = true
    quad_center_elements = false
    show_rgmb_metadata = true
  []
  # Define core
  [core]
    type = CoreMeshGenerator
    inputs = 'fuel_assembly_1 control_assembly reflector_assembly dummy'
    dummy_assembly_name = 'dummy'
    pattern = '      3 2 3;
                    2 0 0 2;
                   3 0 1 0 3;
                    2 0 0 2;
                     3 2 3'
    extrude = true
    show_rgmb_metadata = true
  []
[]

[Debug]
  show_actions = true
[]

[Problem]
  solve = false
[]

[Outputs]
  [out]
    type = Exodus
    execute_on = timestep_end
    output_extra_element_ids = true
    extra_element_ids_to_output = 'region_id'
  []
[]

[Executioner]
  type = Steady
[]
