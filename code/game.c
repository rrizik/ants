// 3,800,000,000 cycles per frame
// 12 cores.
// 63,333,333 cycles 60 frames 1 second 1 core. 1,520,000,000 cycles 12 core.
// 126,666,666 cycles 30 frames 1 second 1 core. 760,000,000 cycles 12 core.
// threads
// whats the value of {}

// 160,000,000 11fps
// 133,930,677
// 120,170,098

#include "game.h"

static void
draw_commands(RenderBuffer *render_buffer, Arena *commands){
    void* at = commands->base;
    void* end = (char*)commands->base + commands->used;
    while(at != end){
        BaseCommand* base_command = (BaseCommand*)at;

        switch(base_command->type){
            case RenderCommand_ClearColor:{
                ClearColorCommand *command = (ClearColorCommand*)base_command;
                clear(render_buffer, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Pixel:{
                PixelCommand *command = (PixelCommand*)base_command;
                draw_pixel(render_buffer, command->x, command->y, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Segment:{
                SegmentCommand *command = (SegmentCommand*)base_command;
                draw_segment(render_buffer, command->p0, command->p1, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Ray:{
                RayCommand *command = (RayCommand*)base_command;
                draw_ray(render_buffer, command->base.position, command->direction, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Line:{
                LineCommand *command = (LineCommand*)base_command;
                draw_line(render_buffer, command->base.position, command->direction, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Rect:{
                RectCommand *command = (RectCommand*)base_command;
                draw_rect(render_buffer, command->base.position, command->dimension, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Box:{
                BoxCommand *command = (BoxCommand*)base_command;
                draw_box(render_buffer, command->base.position, command->dimension, command->base.color);
                at = command + 1;
            } break;
            case RenderCommand_Quad:{
                QuadCommand *command = (QuadCommand*)base_command;
                draw_quad(render_buffer, command->p0, command->p1, command->p2, command->p3, command->base.color, command->base.fill);
                at = command + 1;
            } break;
            case RenderCommand_Triangle:{
                TriangleCommand *command = (TriangleCommand*)base_command;
                draw_triangle(render_buffer, command->p0, command->p1, command->p2, base_command->color, base_command->fill);
                at = command + 1;
            } break;
            case RenderCommand_Circle:{
                CircleCommand *command = (CircleCommand*)base_command;
                draw_circle(render_buffer, command->base.position.x, command->base.position.y, command->rad, command->base.color, command->base.fill);
                at = command + 1;
            } break;
            case RenderCommand_Bitmap:{
                BitmapCommand *command = (BitmapCommand*)base_command;
                draw_bitmap(render_buffer, command->base.position, command->image);
                at = command + 1;
            } break;
        }
    }
}

GameMemory *debug_global_memory;
MAIN_GAME_LOOP(main_game_loop){
    debug_global_memory = memory;
    BEGIN_CYCLE_COUNTER(frame);
    BEGIN_TICK_COUNTER(frame);
#if RDTSC
    clock->cpu_now = __rdtsc();
    f32 frame_cycles = get_cycles_elapsed(clock->cpu_now, clock->cpu_prev);
    clock->cpu_prev = clock->cpu_now;
#endif
    assert(sizeof(PermanentState) <= memory->permanent_storage_size);
    assert(sizeof(TransientState) <= memory->transient_storage_size);
    PermanentState *state = (PermanentState *)memory->permanent_storage;
    TransientState *transient_state = (TransientState *)memory->transient_storage;
#if TICKS
    state->frame_ticks_now = clock->get_ticks();
    f32 frame_ticks = clock->get_ms_elapsed(state->frame_ticks_prev, state->frame_ticks_now); 
    state->frame_ticks_prev = state->frame_ticks_now;
#endif

    v4 RED =     {1.0f, 0.0f, 0.0f,  0.7f};
    v4 GREEN =   {0.0f, 1.0f, 0.0f,  0.7f};
    v4 BLUE =    {0.0f, 0.0f, 1.0f,  0.5f};
    v4 MAGENTA = {1.0f, 0.0f, 1.0f,  0.1f};
    v4 TEAL =    {0.0f, 1.0f, 1.0f,  0.1f};
    v4 PINK =    {0.92f, 0.62f, 0.96f, 0.5f};
    v4 YELLOW =  {0.9f, 0.9f, 0.0f,  0.5f};
    v4 ORANGE =  {1.0f, 0.5f, 0.15f,  0.5f};
    v4 DGRAY =   {0.5f, 0.5f, 0.5f,  0.5f};
    v4 LGRAY =   {0.8f, 0.8f, 0.8f,  0.7f};
    v4 WHITE =   {1.0f, 1.0f, 1.0f,  1.0f};
    v4 BLACK =   {0.0f, 0.0f, 0.0f,  1.0f};

    if(!memory->initialized){
        BEGIN_CYCLE_COUNTER(init);
        BEGIN_TICK_COUNTER(init);

        initialize_arena(&state->permanent_arena,
                         (u8*)memory->permanent_storage + sizeof(PermanentState),
                         memory->permanent_storage_size - sizeof(PermanentState));
        initialize_arena(&transient_state->transient_arena,
                         (u8*)memory->transient_storage + sizeof(TransientState),
                         memory->transient_storage_size - sizeof(TransientState));

        transient_state->render_commands_arena = allocate_arena(
                                                       &transient_state->transient_arena,
                                                       Megabytes(16));
        transient_state->LL_arena = allocate_arena(
                                        &transient_state->transient_arena,
                                        Megabytes(800));


        //init
        state->screen_width = render_buffer->width;
        state->screen_height = render_buffer->height;

#if DEBUG
        seed_random(1, 0);
        print("1\n");
#else
        seed_random(time(NULL), 0);
        print("2\n");
#endif

        state->cell_row_count = 16;
        state->cell_width = render_buffer->width / state->cell_row_count;
        state->cell_height = render_buffer->height / state->cell_row_count;
        //state->ants_count = 3000;
        //state->ants_count = 1000;
        state->ants_count = 500;
        //state->ants_count = 100;
        //state->ants_count = 10;
        //state->ants_count = 1;
        //state->free_entities_size = 110000;
        state->free_entities_at = array_count(state->free_entities) - 1;

        Entity *zero_entity = add_entity(state, EntityType_None);

        state->test = load_bitmap(memory, "test.bmp");
        state->circle = load_bitmap(memory, "circle.bmp");
        state->image = load_bitmap(memory, "image.bmp");

        // setup free entities array
        for(i32 i = array_count(state->free_entities) - 1; i >= 0; --i){
            state->free_entities[i] = array_count(state->free_entities) - 1 - i;
        }

        state->colony = add_colony(state, vec2(render_buffer->width/2, 100), 25, DGRAY, true);
        //state->colony = add_colony(state, vec2(render_buffer->width/2, render_buffer->height/2), 25, DGRAY, true);
        // add ants
        for(u32 i=0; i < state->ants_count; ++i){
            Entity* ant = add_ant(state, state->colony->position, 2, LGRAY, true);
            ant->direction_change_timer = clock->get_ticks();
            ant->pheromone_spawn_timer = clock->get_ticks();
        }

        // add grid
        //for(i32 i=0; i<state->cell_row_count; ++i){
        //    add_segment(state, vec2(0, render_buffer->height - state->cell_height * i), vec2(render_buffer->width, render_buffer->height - state->cell_height * i), DGRAY);
        //    add_segment(state, vec2(render_buffer->width - state->cell_width * i, 0), vec2(render_buffer->width - state->cell_width * i, render_buffer->height), DGRAY);
        //}
        //state->player = add_player(state, vec2(100, 100), vec2(20, 20), ORANGE, state->image);
        state->border_size = 10;
        add_segment(state, vec2(state->border_size, state->border_size), vec2(render_buffer->width - state->border_size, state->border_size), RED);
        add_segment(state, vec2(state->border_size, state->border_size), vec2(state->border_size, render_buffer->height - state->border_size), RED);
        add_segment(state, vec2(render_buffer->width - state->border_size, render_buffer->height - state->border_size), vec2(state->border_size, render_buffer->height - state->border_size), RED);
        add_segment(state, vec2(render_buffer->width - state->border_size, render_buffer->height - state->border_size), vec2(render_buffer->width - state->border_size, state->border_size), RED);

        state->add_food = false;
		state->draw_home_phers = true;
		state->draw_food_phers = true;
		state->draw_depositing_ants = true;
		state->draw_wondering_ants = true;
        memory->initialized = true;
        END_TICK_COUNTER(init);
        END_CYCLE_COUNTER(init);
    }

    BEGIN_CYCLE_COUNTER(reset_sentinels);
    BEGIN_TICK_COUNTER(reset_sentinels);
    // reset LL
    reset_LL_sentinel(&state->ants);
    reset_LL_sentinel(&state->misc);
    for(i32 j=0; j < state->cell_row_count; ++j){
        for(i32 k=0; k < state->cell_row_count; ++k){
            reset_LL_sentinel(&state->food_cells[j][k]);
            reset_LL_sentinel(&state->pher_cells[j][k]);
            reset_LL_sentinel(&state->pher_food_cells[j][k]);
            reset_LL_sentinel(&state->pher_home_cells[j][k]);
        }
    }
    transient_state->LL_arena->used = 0;
    END_TICK_COUNTER(reset_sentinels);
    END_CYCLE_COUNTER(reset_sentinels);

    //print("free_entities: %i\n", state->free_entities_at);
    BEGIN_CYCLE_COUNTER(LL_setup);
    BEGIN_TICK_COUNTER(LL_setup);
    // setup LL
    for(u32 entity_index = 0; entity_index < array_count(state->entities); ++entity_index){
        Entity *e = state->entities + entity_index;
        switch(e->type){
            case EntityType_Box:{
                remove_entity(state, e);
            }break;
            case EntityType_Segment:{
                LinkedList* node = allocate_LL_node(transient_state->LL_arena);
                node->data = e;
                LL_push(&state->misc, node);
            }break;
            case EntityType_Ant:{
                LinkedList* node = allocate_LL_node(transient_state->LL_arena);
                node->data = e;
                LL_push(&state->ants, node);
            }break;
            case EntityType_Food:{
                if(!e->food_targeted && !e->food_collected){
                    v2 cell_coord = vec2(floor(e->position.x / state->cell_width), floor(e->position.y / state->cell_height));
                    cell_coord.x = clamp_f32(0, cell_coord.x, state->cell_row_count - 1);
                    cell_coord.y = clamp_f32(0, cell_coord.y, state->cell_row_count - 1);
                    LinkedList* cell = &state->food_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                    LinkedList* node = allocate_LL_node(transient_state->LL_arena);
                    node->data = e;
                    LL_push(cell, node);
                }
            }break;
            case EntityType_ToHomePheromone:{
                v2 cell_coord = vec2(floor(e->position.x / state->cell_width), floor(e->position.y / state->cell_height));
                cell_coord.x = clamp_f32(0, cell_coord.x, state->cell_row_count - 1);
                cell_coord.y = clamp_f32(0, cell_coord.y, state->cell_row_count - 1);

                LinkedList* cell = &state->pher_home_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                LinkedList* node = allocate_LL_node(transient_state->LL_arena);
                node->data = e;
                LL_push(cell, node);

                LinkedList* pher_cell = &state->pher_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                LinkedList* pher_node = allocate_LL_node(transient_state->LL_arena);
                pher_node->data = e;
                LL_push(pher_cell, pher_node);
            }break;
            case EntityType_ToFoodPheromone:{
                v2 cell_coord = vec2(floor(e->position.x / state->cell_width), floor(e->position.y / state->cell_height));
                cell_coord.x = clamp_f32(0, cell_coord.x, state->cell_row_count - 1);
                cell_coord.y = clamp_f32(0, cell_coord.y, state->cell_row_count - 1);

                LinkedList* cell = &state->pher_food_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                LinkedList* node = allocate_LL_node(transient_state->LL_arena);
                node->data = e;
                LL_push(cell, node);

                LinkedList* pher_cell = &state->pher_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                LinkedList* pher_node = allocate_LL_node(transient_state->LL_arena);
                pher_node->data = e;
                LL_push(pher_cell, pher_node);
            }break;
        }
    }
    END_TICK_COUNTER(LL_setup);
    END_CYCLE_COUNTER(LL_setup);
    BEGIN_CYCLE_COUNTER(events);
    BEGIN_TICK_COUNTER(events);
    // keyboard/controller/mouse events
    for(u32 i=0; i < events->index; ++i){
        Event *event = &events->event[i];
        if(event->type == EVENT_MOUSEMOTION){
            state->controller.mouse_pos = vec2(event->mouse_x, render_buffer->height - event->mouse_y);
        }
        if(event->type == EVENT_MOUSEDOWN){
            if(event->mouse == MOUSE_LBUTTON){ state->controller.m1 = true; }
            if(event->mouse == MOUSE_RBUTTON){ state->controller.m2 = true; }
            if(event->mouse == MOUSE_MBUTTON){ state->controller.m3 = true; }
        }
        if(event->type == EVENT_MOUSEUP){
            if(event->mouse == MOUSE_LBUTTON){ state->controller.m1 = false; }
            if(event->mouse == MOUSE_RBUTTON){ state->controller.m2 = false; }
            if(event->mouse == MOUSE_MBUTTON){ state->controller.m3 = false; }
        }
        if(event->type == EVENT_KEYDOWN){
            if(event->key == KEY_W){ state->controller.up = true; }
            if(event->key == KEY_S){ state->controller.down = true; }
            if(event->key == KEY_A){ state->controller.left = true; }
            if(event->key == KEY_D){ state->controller.right = true; }
            if(event->key == KEY_1){ state->draw_food_phers = !state->draw_food_phers; }
            if(event->key == KEY_2){ state->draw_home_phers = !state->draw_home_phers; }
            if(event->key == KEY_3){ state->draw_wondering_ants = !state->draw_wondering_ants; }
            if(event->key == KEY_4){ state->draw_depositing_ants = !state->draw_depositing_ants; }
        }
        if(event->type == EVENT_KEYUP){
            if(event->key == KEY_ESCAPE){
                memory->running = false;
            }
            if(event->key == KEY_W){
                state->controller.up = false;
            }
            if(event->key == KEY_S){
                state->controller.down = false;
            }
            if(event->key == KEY_A){
                state->controller.left = false;
            }
            if(event->key == KEY_D){
                state->controller.right = false;
            }
        }
    }
    END_TICK_COUNTER(events);
    END_CYCLE_COUNTER(events);

    BEGIN_CYCLE_COUNTER(controller);
    BEGIN_TICK_COUNTER(controller);
	if(state->controller.up){ state->player->position.y += state->player->speed * clock->dt;}
	if(state->controller.down){ state->player->position.y -= state->player->speed * clock->dt;}
	if(state->controller.right){ state->player->position.x += state->player->speed * clock->dt;}
	if(state->controller.left){ state->player->position.x -= state->player->speed * clock->dt;}

    if(state->controller.m1){
        Entity* pher = add_to_food_pheromone(state, state->controller.mouse_pos, 1, TEAL);
        pher->food_decay_timer = clock->get_ticks();
    }
    if(state->controller.m2){
        Entity* pher = add_to_home_pheromone(state, state->controller.mouse_pos, 1, MAGENTA);
        pher->home_decay_timer = clock->get_ticks();
    }
    if(state->controller.m3){
        if(!state->add_food){
            state->add_food = true;
            v2 cell_coord = vec2(floor(state->controller.mouse_pos.x / state->cell_width), floor(state->controller.mouse_pos.y / state->cell_height));
            cell_coord.x = clamp_f32(0, cell_coord.x, state->cell_row_count - 1);
            cell_coord.y = clamp_f32(0, cell_coord.y, state->cell_row_count - 1);
            i32 x_start = cell_coord.x * state->cell_width;
            i32 y_start = cell_coord.y * state->cell_height;
            for(i32 x = x_start; x < (i32)(x_start + state->cell_width); x+=3){
                for(i32 y = y_start; y < (i32)(y_start + state->cell_height); y+=3){
                    add_food(state, vec2(x, y), 1, GREEN, true);
                }
            }
        }
    }
    else{
        state->add_food = false;
    }
    END_TICK_COUNTER(controller);
    END_CYCLE_COUNTER(controller);

    BEGIN_CYCLE_COUNTER(degrade_pher);
    BEGIN_TICK_COUNTER(degrade_pher);
    // pheromone degradation
    for(i32 y=0; y<state->cell_row_count; ++y){
        for(i32 x=0; x<state->cell_row_count; ++x){
            LinkedList *pher_cell = &state->pher_cells[y][x];
            for(LinkedList* node=pher_cell->next; node != pher_cell; node=node->next){
                Entity *e = node->data;
                switch(e->type){
                    case EntityType_ToFoodPheromone:{
                        u64 now = clock->get_ticks();
                        f32 seconds_elapsed = clock->get_seconds_elapsed(e->food_decay_timer, now);
                        f32 t = seconds_elapsed / e->pher_food_decay_rate;
                        e->color.a = lerp(e->pheromone_alpha_start, 0.0f, t);

                        if(e->color.a <= 0){
                            e->color.a = e->pheromone_alpha_start;
                            remove_entity(state, e);
                        }
                    }break;
                    case EntityType_ToHomePheromone:{
                        u64 now = clock->get_ticks();
                        f32 seconds_elapsed = clock->get_seconds_elapsed(e->home_decay_timer, now);
                        f32 t = seconds_elapsed / e->pher_home_decay_rate;
                        e->color.a = lerp(e->pheromone_alpha_start, 0.0f, t);

                        if(e->color.a <= 0.0f){
                            e->color.a = e->pheromone_alpha_start;
                            remove_entity(state, e);
                        }
                    }break;
                }
            }
        }
    }
    END_TICK_COUNTER(degrade_pher);
    END_CYCLE_COUNTER(degrade_pher);

    BEGIN_CYCLE_COUNTER(behavior);
    BEGIN_TICK_COUNTER(behavior);
    // ant behavior
    for(LinkedList* node=state->ants.next; node != &state->ants; node=node->next){
        BEGIN_CYCLE_COUNTER(state_none);
        BEGIN_TICK_COUNTER(state_none);
        u64 behavior_none_stated_ticks = clock->get_ticks();

        Entity *ant = (Entity*)node->data;

        // reset sensor density
        ant->right_sensor_density = 0.0f;
        ant->forward_sensor_density = 0.0f;
        ant->left_sensor_density = 0.0f;

        // to home/food pheromones
        f32 seconds_elapsed = clock->get_seconds_elapsed(ant->pheromone_spawn_timer, clock->get_ticks());
        if(seconds_elapsed >= ant->pheromone_spawn_timer_max){
            if(ant->ant_state == AntState_Wondering || ant->ant_state == AntState_Collecting){
                Entity* pher = add_to_home_pheromone(state, ant->position, 1, MAGENTA);
                pher->home_decay_timer = clock->get_ticks();
            }
            if(ant->ant_state == AntState_Depositing){
                Entity* pher = add_to_food_pheromone(state, ant->position, 1, TEAL);
                pher->food_decay_timer = clock->get_ticks();
            }
            ant->pheromone_spawn_timer = clock->get_ticks();
        }

        // rotate towards target_direction
        f32 ant_direction_radian = dir_to_rad(ant->direction);
        f32 target_direction_radian = dir_to_rad(ant->target_direction);
        v2 new_lerped_direction = rad_to_dir(lerp_rad(ant_direction_radian, target_direction_radian, ant->rot_percent));
        ant->direction = new_lerped_direction;
        ant->rot_percent += ant->rotate_speed;
        if(ant->rot_percent > 1.0f){
            ant->rotation_complete = true;
            ant->rot_percent = 1.0f;
        }
        else{
            ant->rotation_complete = false;
        }

        // move forward direction
        ant->position.x += (ant->speed * ant->direction.x) * clock->dt;
        ant->position.y += (ant->speed * ant->direction.y) * clock->dt;

        // X wall collision
        if(ant->position.x < state->border_size){
            ant->target_direction.x = 1;
            if(!ant->out_of_bounds_x){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_x = true;
            }
            goto end_of_behavior;
        }
        else if(ant->position.x > render_buffer->width - state->border_size){
            ant->target_direction.x = -1;
            if(!ant->out_of_bounds_x){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_x = true;
            }
            goto end_of_behavior;
        }
        // Y wall collision
        else if(ant->position.y < state->border_size){
            ant->target_direction.y = 1;
            if(!ant->out_of_bounds_y){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_y = true;
            }
            goto end_of_behavior;
        }
        else if(ant->position.y > render_buffer->height - state->border_size){
            ant->target_direction.y = -1;
            if(!ant->out_of_bounds_y){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_y = true;
            }
            goto end_of_behavior;
        }
        else{
            ant->out_of_bounds_x = false;
            ant->out_of_bounds_y = false;
        }

        // center cell and bottom left cell coords
        v2 center_cell_coord = vec2(floor(ant->position.x / state->cell_width), floor(ant->position.y / state->cell_height));
        center_cell_coord.x = clamp_f32(0, center_cell_coord.x, state->cell_row_count - 1);
        center_cell_coord.y = clamp_f32(0, center_cell_coord.y, state->cell_row_count - 1);
        v2 bottom_left_cell_coord = vec2(center_cell_coord.x - 1, center_cell_coord.y - 1);

        // all three sensor rects
        f32 forward_rad = dir_to_rad(ant->direction);
        f32 right_rad = forward_rad + (RAD * ant->sensor_angle);
        f32 left_rad = forward_rad - (RAD * ant->sensor_angle);

        v2 right_direction = rad_to_dir(right_rad);
        v2 left_direction = rad_to_dir(left_rad);

        ant->right_sensor = vec2(ant->position.x + (ant->sensor_distance * right_direction.x), ant->position.y + (ant->sensor_distance * right_direction.y));
        ant->mid_sensor = vec2(ant->position.x + (ant->sensor_distance * ant->direction.x), ant->position.y + (ant->sensor_distance * ant->direction.y));
        ant->left_sensor = vec2(ant->position.x + (ant->sensor_distance * left_direction.x), ant->position.y + (ant->sensor_distance * left_direction.y));

        Rect right_rect = rect(vec2(ant->right_sensor.x - ant->sensor_radius, ant->right_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
        Rect mid_rect = rect(vec2(ant->mid_sensor.x - ant->sensor_radius, ant->mid_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
        Rect left_rect = rect(vec2(ant->left_sensor.x - ant->sensor_radius, ant->left_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));

        END_TICK_COUNTER(state_none);
        END_CYCLE_COUNTER(state_none);
        BEGIN_CYCLE_COUNTER(state_wondering);
        BEGIN_TICK_COUNTER(state_wondering);
        if(ant->ant_state == AntState_Wondering){

            BEGIN_CYCLE_COUNTER(wondering_search);
            BEGIN_TICK_COUNTER(wondering_search);
            Entity *food = NULL;
            Entity *pher = NULL;
            for(i32 y=bottom_left_cell_coord.y; y<=center_cell_coord.y+1; ++y){
                for(i32 x=bottom_left_cell_coord.x; x<=center_cell_coord.x+1; ++x){
                    if(x >= 0 && x < state->cell_row_count && y >= 0 && y < state->cell_row_count){
                        //add_box(state, vec2(state->cell_width * x, state->cell_height * y), vec2(state->cell_width, state->cell_height), ORANGE);
                        BEGIN_CYCLE_COUNTER(wondering_search_food);
                        BEGIN_TICK_COUNTER(wondering_search_food);
                        LinkedList* food_cell = &state->food_cells[y][x];
                        for(LinkedList* node=food_cell->next; node != food_cell; node=node->next){
                            food = node->data;
                            if(!food->food_targeted){
                                f32 distance = distance2(ant->position, food->position);
                                if(distance < 20){
                                    //ant->color = ORANGE;
                                    ant->ant_state = AntState_Collecting;
                                    food->food_targeted = true;
                                    ant->ant_food = handle_from_entity(state, food);
                                    food->food_ant = handle_from_entity(state, ant);

                                    ant->target_direction = direction(ant->position, food->position);
                                    ant->rot_percent = 0.0f;
                                    END_TICK_COUNTER(wondering_search_food);
                                    END_CYCLE_COUNTER(wondering_search_food);
                                    goto end_of_behavior;
                                }
                            }
                        }
                        END_TICK_COUNTER(wondering_search_food);
                        END_CYCLE_COUNTER(wondering_search_food);

                        BEGIN_CYCLE_COUNTER(wondering_search_pher);
                        BEGIN_TICK_COUNTER(wondering_search_pher);
                        LinkedList* pher_cell = &state->pher_food_cells[y][x];
                        for(LinkedList *node=pher_cell->next; node != pher_cell; node=node->next){
                            pher = node->data;
                            if(rect_collides_point(right_rect, pher->position)){
                                ant->right_sensor_density += pher->color.a;
                            }
                            if(rect_collides_point(mid_rect, pher->position)){
                                ant->forward_sensor_density += pher->color.a;
                            }
                            if(rect_collides_point(left_rect, pher->position)){
                                ant->left_sensor_density += pher->color.a;
                            }
                        }
                        END_TICK_COUNTER(wondering_search_pher);
                        END_CYCLE_COUNTER(wondering_search_pher);
                    }
                }
            }
            END_TICK_COUNTER(wondering_search);
            END_CYCLE_COUNTER(wondering_search);

            if(ant->right_sensor_density > 0 || ant->forward_sensor_density > 0 || ant->left_sensor_density > 0){
                if(ant->forward_sensor_density > MAXf32(ant->right_sensor_density, ant->left_sensor_density)){
                    ant->target_direction = ant->direction;
                    if(ant->forward == false){
                        ant->rot_percent = 0.0f;
                    }
                    ant->right = false;
                    ant->forward = true;
                    ant->left = false;
                }
                else if(ant->right_sensor_density > ant->left_sensor_density){
                    ant->target_direction = right_direction;
                    if(ant->right == false){
                        ant->rot_percent = 0.0f;
                    }
                    ant->right = true;
                    ant->forward = false;
                    ant->left = false;
                }
                else if(ant->left_sensor_density > ant->right_sensor_density){
                    ant->target_direction = left_direction;
                    if(ant->left == false){
                        ant->rot_percent = 0.0f;
                    }
                    ant->right = false;
                    ant->forward = false;
                    ant->left = true;
                }
            }
            else{
                // random target_direction on timer
                f32 seconds_elapsed = clock->get_seconds_elapsed(ant->direction_change_timer, clock->get_ticks());
                if(seconds_elapsed >= ant->direction_change_timer_max){
                    ant->random_vector.x = ((i32)(random_range(201)-100)/100.0f);
                    ant->random_vector.y = ((i32)(random_range(201)-100)/100.0f);
                    ant->rot_percent = 0.0f;
                    ant->direction_change_timer_max = (random_range(3) + 1);
                    ant->direction_change_timer = clock->get_ticks();
                    ant->target_direction = ant->random_vector;
                }
            }
            END_TICK_COUNTER(state_wondering);
            END_CYCLE_COUNTER(state_wondering);
        }

        else if(ant->ant_state == AntState_Collecting){
            BEGIN_CYCLE_COUNTER(state_collecting);
            BEGIN_TICK_COUNTER(state_collecting);

            Entity* targeted_food = entity_from_handle(state, ant->ant_food);
            ant->target_direction = direction(ant->position, targeted_food->position);
            f32 distance = distance2(ant->position, targeted_food->position);

            // consume food once close enough
            if(distance < 2){
                ant->ant_state = AntState_Depositing;
                targeted_food->food_collected = true;
                ant->target_direction.x = -ant->direction.x;
                ant->target_direction.y = -ant->direction.y;
                ant->rot_percent = 0.0f;
                ant->direction_change_timer_max = (random_range(3) + 1);
                ant->direction_change_timer = clock->get_ticks();
                ant->color = RED;
            }
            END_TICK_COUNTER(state_collecting);
            END_CYCLE_COUNTER(state_collecting);
        }

        else if(ant->ant_state == AntState_Depositing){
            BEGIN_CYCLE_COUNTER(state_depositing);
            BEGIN_TICK_COUNTER(state_depositing);

            Entity* collected_food = entity_from_handle(state, ant->ant_food);
            collected_food->position.x = ant->position.x + (5 * ant->direction.x);
            collected_food->position.y = ant->position.y + (5 * ant->direction.y);

            if(!ant->colony_targeted){
                BEGIN_CYCLE_COUNTER(depositing_search);
                BEGIN_TICK_COUNTER(depositing_search);
                Entity *pher = NULL;
                for(i32 y=bottom_left_cell_coord.y; y<=center_cell_coord.y+1; ++y){
                    for(i32 x=bottom_left_cell_coord.x; x<=center_cell_coord.x+1; ++x){
                        if(x >= 0 && x < state->cell_row_count && y >= 0 && y < state->cell_row_count){
                            LinkedList* pher_cell = &state->pher_home_cells[y][x];
                            for(LinkedList* node=pher_cell->next; node != pher_cell; node=node->next){
                                pher = node->data;
                                if(rect_collides_point(right_rect, pher->position)){
                                    ant->right_sensor_density += pher->color.a;
                                }
                                if(rect_collides_point(mid_rect, pher->position)){
                                    ant->forward_sensor_density += pher->color.a;
                                }
                                if(rect_collides_point(left_rect, pher->position)){
                                    ant->left_sensor_density += pher->color.a;
                                }
                            }
                        }
                    }
                }
                END_TICK_COUNTER(depositing_search);
                END_CYCLE_COUNTER(depositing_search);

                if(ant->right_sensor_density > 0 || ant->forward_sensor_density > 0 || ant->left_sensor_density > 0){
                    if(ant->forward_sensor_density > MAXf32(ant->right_sensor_density, ant->left_sensor_density)){
                        ant->target_direction = ant->direction;
                        if(ant->forward == false){
                            ant->rot_percent = 0.0f;
                        }
                        ant->right = false;
                        ant->forward = true;
                        ant->left = false;
                    }
                    else if(ant->right_sensor_density > ant->left_sensor_density){
                        ant->target_direction = right_direction;
                        if(ant->right == false){
                            ant->rot_percent = 0.0f;
                        }
                        ant->right = true;
                        ant->forward = false;
                        ant->left = false;
                    }
                    else if(ant->left_sensor_density > ant->right_sensor_density){
                        ant->target_direction = left_direction;
                        if(ant->left == false){
                            ant->rot_percent = 0.0f;
                        }
                        ant->right = false;
                        ant->forward = false;
                        ant->left = true;
                    }
                }
                else{
                    // random target_direction on timer
                    f32 seconds_elapsed = clock->get_seconds_elapsed(ant->direction_change_timer, clock->get_ticks());
                    if(seconds_elapsed >= ant->direction_change_timer_max){
                        ant->random_vector.x = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                        ant->random_vector.y = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                        ant->rot_percent = 0.0f;
                        ant->direction_change_timer_max = (random_range(3) + 1);
                        ant->direction_change_timer = clock->get_ticks();
                        ant->target_direction = ant->random_vector;
                    }
                }
            }

            // target colony when close enough
            f32 colony_distance = distance2(ant->position, state->colony->position);
            if(colony_distance < 75){
                ant->target_direction = direction2(ant->position, state->colony->position);
                if(!ant->colony_targeted){
                    ant->rot_percent = 0;
                    ant->colony_targeted = true;
                }
            }

            // deposite if close to colony
            if(colony_distance < 2){
                remove_entity(state, collected_food);

                ant->ant_state = AntState_Wondering;
                ant->target_direction.x = -ant->direction.x;
                ant->target_direction.y = -ant->direction.y;
                ant->colony_targeted = false;
                ant->rot_percent = 0.0f;
                ant->direction_change_timer_max = (random_range(3) + 1);
                ant->direction_change_timer = clock->get_ticks();
                collected_food->food_targeted = false;
                collected_food->food_collected = false;
                collected_food->food_ant = zero_entity_handle();
                ant->ant_food = zero_entity_handle();
                ant->color = LGRAY;
            }
            END_TICK_COUNTER(state_depositing);
            END_CYCLE_COUNTER(state_depositing);
        }
        end_of_behavior:;
    }
    END_TICK_COUNTER(behavior);
    END_CYCLE_COUNTER(behavior);

    BEGIN_CYCLE_COUNTER(allocate_commands);
    BEGIN_TICK_COUNTER(allocate_commands);
    transient_state->render_commands_arena->used = 0;
    allocate_clear_color(transient_state->render_commands_arena, BLACK);
    for(u32 entity_index = 0; entity_index <= array_count(state->entities); ++entity_index){
        Entity *e = state->entities + entity_index;
        switch(e->type){
            case EntityType_Player:{
                //allocate_bitmap(transient_state->render_commands_arena, e->position, e->image);
                //if(e->draw_bounding_box){
                //    allocate_box(transient_state->render_commands_arena, e->position, e->dimension, e->color);
                //}
                //allocate_rect(transient_state->render_commands_arena, e->position, e->dimension, e->color);
                allocate_box(transient_state->render_commands_arena, e->position, e->dimension, e->color);
            }break;
            case EntityType_Pixel:{
                allocate_pixel(transient_state->render_commands_arena, e->position, e->color);
            }break;
            case EntityType_Segment:{
                allocate_segment(transient_state->render_commands_arena, e->p0, e->p1, e->color);
            }break;
            case EntityType_Line:{
                allocate_line(transient_state->render_commands_arena, e->position, e->direction, e->color);
            }break;
            case EntityType_Ray:{
                allocate_ray(transient_state->render_commands_arena, e->position, e->direction, e->color);
            }break;
            case EntityType_Rect:{
                allocate_rect(transient_state->render_commands_arena, e->position, e->dimension, e->color);
            }break;
            case EntityType_Box:{
                allocate_box(transient_state->render_commands_arena, e->position, e->dimension, e->color);
            }break;
            case EntityType_Quad:{
                allocate_quad(transient_state->render_commands_arena, e->p0, e->p1, e->p2, e->p3, e->color, e->fill);
            }break;
            case EntityType_Triangle:{
                allocate_triangle(transient_state->render_commands_arena, e->p0, e->p1, e->p2, e->color, e->fill);
            }break;
            case EntityType_Circle:{
                allocate_circle(transient_state->render_commands_arena, e->position, e->rad, e->color, e->fill);
                v2 dimenstion = vec2(e->rad*2, e->rad*2);
                v2 position = vec2(e->position.x - e->rad, e->position.y - e->rad);
                if(e->draw_bounding_box){
                    allocate_box(transient_state->render_commands_arena, position, dimenstion, e->color);
                }
            }break;
            case EntityType_Bitmap:{
                allocate_bitmap(transient_state->render_commands_arena, e->position, e->image);
            }break;
            case EntityType_None:{
            }break;
            case EntityType_Object:{
            }break;
            case EntityType_Food:{
                if(!e->food_collected){
                    allocate_circle(transient_state->render_commands_arena, e->position, e->rad, e->color, e->fill);
                }
                if(state->draw_depositing_ants){
                    if(e->food_collected){
                        allocate_circle(transient_state->render_commands_arena, e->position, e->rad, e->color, e->fill);
                    }
                }
            }break;
            case EntityType_Ant:{
				if(state->draw_wondering_ants && equal4(e->color, LGRAY)){
					allocate_circle(transient_state->render_commands_arena, e->position, e->rad, e->color, e->fill);
				}
                if(state->draw_depositing_ants && equal4(e->color, RED)){
					allocate_circle(transient_state->render_commands_arena, e->position, e->rad, e->color, e->fill);
				}
                if(equal4(e->color, ORANGE)){
					allocate_circle(transient_state->render_commands_arena, e->position, e->rad, e->color, e->fill);
                }

                f32 forward_rad = dir_to_rad(e->direction);
                f32 right_rad = forward_rad + (RAD * e->sensor_angle);
                f32 left_rad = forward_rad - (RAD * e->sensor_angle);

                v2 right_direction = rad_to_dir(right_rad);
                v2 left_direction = rad_to_dir(left_rad);

                e->right_sensor = vec2(e->position.x + (e->sensor_distance * right_direction.x), e->position.y + (e->sensor_distance * right_direction.y));
                e->mid_sensor = vec2(e->position.x + (e->sensor_distance * e->direction.x), e->position.y + (e->sensor_distance * e->direction.y));
                e->left_sensor = vec2(e->position.x + (e->sensor_distance * left_direction.x), e->position.y + (e->sensor_distance * left_direction.y));

                Rect right_rect = rect(vec2(e->right_sensor.x - e->sensor_radius, e->right_sensor.y - e->sensor_radius), vec2(e->sensor_radius * 2, e->sensor_radius * 2));
                Rect mid_rect = rect(vec2(e->mid_sensor.x - e->sensor_radius, e->mid_sensor.y - e->sensor_radius), vec2(e->sensor_radius * 2, e->sensor_radius * 2));
                Rect left_rect = rect(vec2(e->left_sensor.x - e->sensor_radius, e->left_sensor.y - e->sensor_radius), vec2(e->sensor_radius * 2, e->sensor_radius * 2));

                //allocate_box(transient_state->render_commands_arena, right_rect.position, right_rect.dimension, YELLOW);
                //allocate_box(transient_state->render_commands_arena, mid_rect.position, mid_rect.dimension, GREEN);
                //allocate_box(transient_state->render_commands_arena, left_rect.position, left_rect.dimension, ORANGE);

                //allocate_circle(transient_state->render_commands_arena, e->right_sensor, e->sensor_radius, YELLOW, false);
                //allocate_circle(transient_state->render_commands_arena, e->mid_sensor, e->sensor_radius, GREEN, false);
                //allocate_circle(transient_state->render_commands_arena, e->left_sensor, e->sensor_radius, ORANGE, false);

                //allocate_segment(transient_state->render_commands_arena, e->position, e->right_sensor, YELLOW);
                //allocate_segment(transient_state->render_commands_arena, e->position, e->mid_sensor, GREEN);
                //allocate_segment(transient_state->render_commands_arena, e->position, e->left_sensor, ORANGE);
                //allocate_ray(transient_state->render_commands_arena, e->position, e->target_direction, BLUE);
            }break;
            case EntityType_Colony:{
                allocate_circle(transient_state->render_commands_arena, e->position, e->rad, e->color, e->fill);
            }break;
            case EntityType_ToHomePheromone:{
                if(state->draw_home_phers){
                    allocate_circle(transient_state->render_commands_arena, e->position, e->rad, e->color, true);
                }
            }break;
            case EntityType_ToFoodPheromone:{
                if(state->draw_food_phers){
                    allocate_circle(transient_state->render_commands_arena, e->position, e->rad, e->color, true);
                }
            }break;
        }
    }
    END_TICK_COUNTER(allocate_commands);
    END_CYCLE_COUNTER(allocate_commands);
    BEGIN_CYCLE_COUNTER(draw);
    BEGIN_TICK_COUNTER(draw);
    draw_commands(render_buffer, transient_state->render_commands_arena);
    END_TICK_COUNTER(draw);
    END_CYCLE_COUNTER(draw);
    state->frame_index++;
    //print("used_bytes: %i\n", transient_state->render_commands_arena->used_bytes);
    //print("-----------------------------------\n");
    END_TICK_COUNTER(frame);
    END_CYCLE_COUNTER(frame);
}
