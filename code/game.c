#include "game.h"

#define RDTSC 0
// ant logic can be simplified dramatically
// ants are not turning around properly (maybe fixed)
// try having ants change direction only after they have completed a rotation
// threads
// whats the value of {}
// entity handles

static void
draw_commands(RenderBuffer *render_buffer, RenderCommandBuffer *commands){
    void* at = commands->base;
    void* end = (char*)commands->base + commands->used_bytes;
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

MAIN_GAME_LOOP(main_game_loop){
#if RDTSC
    clock->cpu_now = __rdtsc();
    f32 frame_cycles = get_cpu_cycles_elapsed(clock->cpu_now, clock->cpu_prev);
    clock->cpu_prev = clock->cpu_now;
#endif

    v4 RED =     {1.0f, 0.0f, 0.0f,  0.5f};
    v4 GREEN =   {0.0f, 1.0f, 0.0f,  0.5f};
    v4 BLUE =    {0.0f, 0.0f, 1.0f,  0.5f};
    v4 MAGENTA = {1.0f, 0.0f, 1.0f,  0.1f};
    v4 PINK =    {0.92f, 0.62f, 0.96f, 0.5f};
    v4 YELLOW =  {0.9f, 0.9f, 0.0f,  0.5f};
    v4 TEAL =    {0.0f, 1.0f, 1.0f,  0.1f};
    v4 ORANGE =  {1.0f, 0.5f, 0.15f,  0.5f};
    v4 DGRAY =   {0.5f, 0.5f, 0.5f,  0.5f};
    v4 LGRAY =   {0.8f, 0.8f, 0.8f,  0.5f};
    v4 WHITE =   {1.0f, 1.0f, 1.0f,  1.0f};
    v4 BLACK =   {0.0f, 0.0f, 0.0f,  1.f};

    assert(sizeof(GameState) <= memory->permanent_storage_size);
    assert(sizeof(TranState) <= memory->transient_storage_size);
    GameState *game_state = (GameState *)memory->permanent_storage;
    TranState *transient_state = (TranState *)memory->transient_storage;

    if(!memory->initialized){
        u64 init_cpu_length = __rdtsc();
        initialize_arena(&game_state->permanent_arena,
                         (u8*)memory->permanent_storage + sizeof(GameState),
                         memory->permanent_storage_size - sizeof(GameState));
        initialize_arena(&transient_state->transient_arena,
                         (u8*)memory->transient_storage + sizeof(TranState),
                         memory->transient_storage_size - sizeof(TranState));

        transient_state->render_commands_buffer = allocate_render_commands_buffer(
                                                       &transient_state->transient_arena,
                                                       Megabytes(16));
        transient_state->LL_buffer = allocate_LL_buffer(
                                        &transient_state->transient_arena,
                                        Megabytes(800));


        //init
        game_state->screen_width = render_buffer->width;
        game_state->screen_height = render_buffer->height;

        //seed_random(time(NULL), 0);
        seed_random(1, 0);

        game_state->cell_row_count = 16;
        game_state->cell_width = render_buffer->width / game_state->cell_row_count;
        game_state->cell_height = render_buffer->height / game_state->cell_row_count;
        //game_state->ants_count = 3000;
        //game_state->ants_count = 1000;
        game_state->ants_count = 500;
        //game_state->ants_count = 100;
        //game_state->ants_count = 10;
        //game_state->ants_count = 1;
        game_state->entities_size = 110000;
        game_state->free_entities_size = 110000;
        game_state->free_entities_at = game_state->free_entities_size - 1;

        Entity *none_entity = add_entity(game_state, EntityType_None);

        game_state->test = load_bitmap(memory, "test.bmp");
        game_state->circle = load_bitmap(memory, "circle.bmp");
        game_state->image = load_bitmap(memory, "image.bmp");

        // setup free entities array
        for(i32 i = game_state->free_entities_size - 1; i >= 0; --i){
            game_state->free_entities[i] = game_state->free_entities_size - 1 - i;
        }

        // add ants
        for(u32 i=0; i < game_state->ants_count; ++i){
            Entity* ant = add_ant(game_state, game_state->colony->position, 2, LGRAY, true);
            ant->direction_change_timer = clock->get_ticks();
            ant->pheromone_spawn_timer = clock->get_ticks();
        }

        // add grid
        //for(i32 i=0; i<game_state->cell_row_count; ++i){
        //    add_segment(game_state, vec2(0, render_buffer->height - game_state->cell_height * i), vec2(render_buffer->width, render_buffer->height - game_state->cell_height * i), DGRAY);
        //    add_segment(game_state, vec2(render_buffer->width - game_state->cell_width * i, 0), vec2(render_buffer->width - game_state->cell_width * i, render_buffer->height), DGRAY);
        //}
        //game_state->player = add_player(game_state, vec2(100, 100), vec2(20, 20), ORANGE, game_state->image);
        game_state->border_size = 10;
        add_segment(game_state, vec2(game_state->border_size, game_state->border_size), vec2(render_buffer->width - game_state->border_size, game_state->border_size), RED);
        add_segment(game_state, vec2(game_state->border_size, game_state->border_size), vec2(game_state->border_size, render_buffer->height - game_state->border_size), RED);
        add_segment(game_state, vec2(render_buffer->width - game_state->border_size, render_buffer->height - game_state->border_size), vec2(game_state->border_size, render_buffer->height - game_state->border_size), RED);
        add_segment(game_state, vec2(render_buffer->width - game_state->border_size, render_buffer->height - game_state->border_size), vec2(render_buffer->width - game_state->border_size, game_state->border_size), RED);

        game_state->add_food = false;
		game_state->draw_home_phers = true;
		game_state->draw_food_phers = true;
		game_state->draw_depositing_ants = true;
		game_state->draw_wondering_ants = true;
        memory->initialized = true;
#if RDTSC
        f32 cycles = get_cpu_cycles_elapsed(__rdtsc(), init_cpu_length);
        print("init_cycles: %0.0f\n", cycles);
    }

    print("-----------------------------------------\n");

    u64 cpu_start = __rdtsc();
#else
    }
#endif

    // reset LL
    reset_LL_sentinel(&game_state->collected_foods);
    reset_LL_sentinel(&game_state->ants);
    reset_LL_sentinel(&game_state->misc);
    for(i32 j=0; j < game_state->cell_row_count; ++j){
        for(i32 k=0; k < game_state->cell_row_count; ++k){
            reset_LL_sentinel(&game_state->food_cells[j][k]);
            reset_LL_sentinel(&game_state->pher_cells[j][k]);
            reset_LL_sentinel(&game_state->pher_food_cells[j][k]);
            reset_LL_sentinel(&game_state->pher_home_cells[j][k]);
        }
    }
    transient_state->LL_buffer->used_bytes = 0;
#if RDTSC
    f32 reset_sentinel_cycles = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);

    cpu_start = __rdtsc();
#endif

    // setup LL
    game_state->collecting_count = 0;
    game_state->food_count = 0;
    for(u32 entity_index = 0; entity_index < game_state->entities_size; ++entity_index){
        Entity *e = game_state->entities + entity_index;
        switch(e->type){
            case EntityType_Box:{
                e->type = EntityType_None;
                game_state->free_entities_at++;
                game_state->free_entities[game_state->free_entities_at] = e->index;
            }break;
            case EntityType_Segment:{
                LinkedList* node = allocate_LL_node(transient_state->LL_buffer);
                node->data = e;
                LL_push(&game_state->misc, node);
            }break;
            case EntityType_Ant:{
                LinkedList* node = allocate_LL_node(transient_state->LL_buffer);
                node->data = e;
                LL_push(&game_state->ants, node);
                if(e->ant_state == AntState_Collecting){
                    game_state->collecting_count++;
                }
            }break;
            case EntityType_Food:{
                if(!e->food_targeted && !e->food_collected){
                    v2 cell_coord = vec2(floor(e->position.x / game_state->cell_width), floor(e->position.y / game_state->cell_height));
                    cell_coord.x = clamp_f32(0, cell_coord.x, game_state->cell_row_count - 1);
                    cell_coord.y = clamp_f32(0, cell_coord.y, game_state->cell_row_count - 1);
                    LinkedList* cell = &game_state->food_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                    LinkedList* node = allocate_LL_node(transient_state->LL_buffer);
                    node->data = e;
                    LL_push(cell, node);
                    game_state->food_count++;
                }

                if(e->food_collected){
                    LinkedList* food_node = allocate_LL_node(transient_state->LL_buffer);
                    food_node->data = e;
                    LL_push(&game_state->collected_foods, food_node);
                }
            }break;
            case EntityType_ToHomePheromone:{
                v2 cell_coord = vec2(floor(e->position.x / game_state->cell_width), floor(e->position.y / game_state->cell_height));
                cell_coord.x = clamp_f32(0, cell_coord.x, game_state->cell_row_count - 1);
                cell_coord.y = clamp_f32(0, cell_coord.y, game_state->cell_row_count - 1);

                LinkedList* cell = &game_state->pher_home_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                LinkedList* node = allocate_LL_node(transient_state->LL_buffer);
                node->data = e;
                LL_push(cell, node);

                LinkedList* pher_cell = &game_state->pher_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                LinkedList* pher_node = allocate_LL_node(transient_state->LL_buffer);
                pher_node->data = e;
                LL_push(pher_cell, pher_node);
            }break;
            case EntityType_ToFoodPheromone:{
                v2 cell_coord = vec2(floor(e->position.x / game_state->cell_width), floor(e->position.y / game_state->cell_height));
                cell_coord.x = clamp_f32(0, cell_coord.x, game_state->cell_row_count - 1);
                cell_coord.y = clamp_f32(0, cell_coord.y, game_state->cell_row_count - 1);

                LinkedList* cell = &game_state->pher_food_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                LinkedList* node = allocate_LL_node(transient_state->LL_buffer);
                node->data = e;
                LL_push(cell, node);

                LinkedList* pher_cell = &game_state->pher_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                LinkedList* pher_node = allocate_LL_node(transient_state->LL_buffer);
                pher_node->data = e;
                LL_push(pher_cell, pher_node);
            }break;
        }
    }
    //print("%i - %i\n", game_state->collecting_count, game_state->food_count);
#if RDTSC
    f32 LL_setup = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);

    cpu_start = __rdtsc();
#endif
    // keyboard/controller/mouse events
    for(u32 i=0; i < events->index; ++i){
        Event *event = &events->event[i];
        if(event->type == EVENT_MOUSEMOTION){
            game_state->controller.mouse_pos = vec2(event->mouse_x, render_buffer->height - event->mouse_y);
        }
        if(event->type == EVENT_MOUSEDOWN){
            if(event->mouse == MOUSE_LBUTTON){ game_state->controller.m1 = true; }
            if(event->mouse == MOUSE_RBUTTON){ game_state->controller.m2 = true; }
            if(event->mouse == MOUSE_MBUTTON){ game_state->controller.m3 = true; }
        }
        if(event->type == EVENT_MOUSEUP){
            if(event->mouse == MOUSE_LBUTTON){ game_state->controller.m1 = false; }
            if(event->mouse == MOUSE_RBUTTON){ game_state->controller.m2 = false; }
            if(event->mouse == MOUSE_MBUTTON){ game_state->controller.m3 = false; }
        }
        if(event->type == EVENT_KEYDOWN){
            if(event->key == KEY_W){ game_state->controller.up = true; }
            if(event->key == KEY_S){ game_state->controller.down = true; }
            if(event->key == KEY_A){ game_state->controller.left = true; }
            if(event->key == KEY_D){ game_state->controller.right = true; }
            if(event->key == KEY_1){ game_state->draw_food_phers = !game_state->draw_food_phers; }
            if(event->key == KEY_2){ game_state->draw_home_phers = !game_state->draw_home_phers; }
            if(event->key == KEY_3){ game_state->draw_wondering_ants = !game_state->draw_wondering_ants; }
            if(event->key == KEY_4){ game_state->draw_depositing_ants = !game_state->draw_depositing_ants; }
        }
        if(event->type == EVENT_KEYUP){
            if(event->key == KEY_ESCAPE){
                memory->running = false;
            }
            if(event->key == KEY_W){
                game_state->controller.up = false;
            }
            if(event->key == KEY_S){
                game_state->controller.down = false;
            }
            if(event->key == KEY_A){
                game_state->controller.left = false;
            }
            if(event->key == KEY_D){
                game_state->controller.right = false;
            }
        }
    }
#if RDTSC
    f32 events_cycles = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);

    cpu_start = __rdtsc();
#endif

	if(game_state->controller.up){ game_state->player->position.y += game_state->player->speed * clock->dt;}
	if(game_state->controller.down){ game_state->player->position.y -= game_state->player->speed * clock->dt;}
	if(game_state->controller.right){ game_state->player->position.x += game_state->player->speed * clock->dt;}
	if(game_state->controller.left){ game_state->player->position.x -= game_state->player->speed * clock->dt;}

    if(game_state->controller.m1){
        Entity* pher = add_to_food_pheromone(game_state, game_state->controller.mouse_pos, 1, TEAL);
        pher->food_decay_timer = clock->get_ticks();
    }
    if(game_state->controller.m2){
        Entity* pher = add_to_home_pheromone(game_state, game_state->controller.mouse_pos, 1, MAGENTA);
        pher->home_decay_timer = clock->get_ticks();
    }
    if(game_state->controller.m3){
        if(!game_state->add_food){
            game_state->add_food = true;
            v2 cell_coord = vec2(floor(game_state->controller.mouse_pos.x / game_state->cell_width), floor(game_state->controller.mouse_pos.y / game_state->cell_height));
            cell_coord.x = clamp_f32(0, cell_coord.x, game_state->cell_row_count - 1);
            cell_coord.y = clamp_f32(0, cell_coord.y, game_state->cell_row_count - 1);
            i32 x_start = cell_coord.x * game_state->cell_width;
            i32 y_start = cell_coord.y * game_state->cell_height;
            for(i32 x = x_start; x < (i32)(x_start + game_state->cell_width); x+=3){
                for(i32 y = y_start; y < (i32)(y_start + game_state->cell_height); y+=3){
                    add_food(game_state, vec2(x, y), 1, GREEN, true);
                }
            }
        }
    }
    else{
        game_state->add_food = false;
    }

#if RDTSC
    f32 controller_cycles = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);

    cpu_start = __rdtsc();
#endif
    // pheromone degradation
    for(i32 y=0; y<game_state->cell_row_count; ++y){
        for(i32 x=0; x<game_state->cell_row_count; ++x){
            LinkedList *pher_cell = &game_state->pher_cells[y][x];
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
                            e->type = EntityType_None;
                            game_state->free_entities_at++;
                            game_state->free_entities[game_state->free_entities_at] = e->index;
                        }
                    }break;
                    case EntityType_ToHomePheromone:{
                        u64 now = clock->get_ticks();
                        f32 seconds_elapsed = clock->get_seconds_elapsed(e->home_decay_timer, now);
                        f32 t = seconds_elapsed / e->pher_home_decay_rate;
                        e->color.a = lerp(e->pheromone_alpha_start, 0.0f, t);

                        if(e->color.a <= 0.0f){
                            e->color.a = e->pheromone_alpha_start;
                            e->type = EntityType_None;
                            game_state->free_entities_at++;
                            game_state->free_entities[game_state->free_entities_at] = e->index;
                        }
                    }break;
                }
            }
        }
    }
#if RDTSC
    f32 pher_degredation = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);

    cpu_start = __rdtsc();
#endif

    for(LinkedList* node=game_state->collected_foods.next; node != &game_state->collected_foods; node=node->next){
        Entity *food = (Entity*)node->data;
        Entity *ant = food->food_ant;
        food->position.x = ant->position.x + (5 * ant->direction.x);
        food->position.y = ant->position.y + (5 * ant->direction.y);
    }

    // ant behavior
    for(LinkedList* node=game_state->ants.next; node != &game_state->ants; node=node->next){
        Entity *ant = (Entity*)node->data;
        //print("dir_x: %0.05f - dir_y: %0.05f - rotation_complete: %i\n", ant->direction.x, ant->direction.y, ant->rotation_complete);

        // reset sensor density
        ant->right_sensor_density = 0.0f;
        ant->forward_sensor_density = 0.0f;
        ant->left_sensor_density = 0.0f;

        // to home/food pheromones
        f32 seconds_elapsed = clock->get_seconds_elapsed(ant->pheromone_spawn_timer, clock->get_ticks());
        if(seconds_elapsed >= ant->pheromone_spawn_timer_max){
            if(ant->ant_state == AntState_Wondering || ant->ant_state == AntState_Collecting){
                Entity* pher = add_to_home_pheromone(game_state, ant->position, 1, MAGENTA);
                pher->home_decay_timer = clock->get_ticks();
            }
            if(ant->ant_state == AntState_Depositing){
                Entity* pher = add_to_food_pheromone(game_state, ant->position, 1, TEAL);
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
        if(ant->position.x < game_state->border_size){
            ant->target_direction.x = 1;
            //ant->ant_state = AntState_Wondering;
            if(!ant->out_of_bounds_x){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_x = true;
            }
            goto end_of_behavior;
        }
        else if(ant->position.x > render_buffer->width - game_state->border_size){
            ant->target_direction.x = -1;
            //ant->ant_state = AntState_Wondering;
            if(!ant->out_of_bounds_x){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_x = true;
            }
            goto end_of_behavior;
        }
        // Y wall collision
        else if(ant->position.y < game_state->border_size){
            ant->target_direction.y = 1;
            //ant->ant_state = AntState_Wondering;
            if(!ant->out_of_bounds_y){
                ant->direction_change_timer = clock->get_ticks();
                ant->rot_percent = 0.0f;
                ant->out_of_bounds_y = true;
            }
            goto end_of_behavior;
        }
        else if(ant->position.y > render_buffer->height - game_state->border_size){
            ant->target_direction.y = -1;
            //ant->ant_state = AntState_Wondering;
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
        v2 center_cell_coord = vec2(floor(ant->position.x / game_state->cell_width), floor(ant->position.y / game_state->cell_height));
        center_cell_coord.x = clamp_f32(0, center_cell_coord.x, game_state->cell_row_count - 1);
        center_cell_coord.y = clamp_f32(0, center_cell_coord.y, game_state->cell_row_count - 1);
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

        u64 wondering_cycles = __rdtsc();
        if(ant->ant_state == AntState_Wondering){

            u64 wondering_search_cycles = __rdtsc();
            Entity *food = NULL;
            Entity *pher = NULL;
            for(i32 y=bottom_left_cell_coord.y; y<=center_cell_coord.y+1; ++y){
                for(i32 x=bottom_left_cell_coord.x; x<=center_cell_coord.x+1; ++x){
                    if(x >= 0 && x < game_state->cell_row_count && y >= 0 && y < game_state->cell_row_count){
                        //add_box(game_state, vec2(game_state->cell_width * x, game_state->cell_height * y), vec2(game_state->cell_width, game_state->cell_height), ORANGE);
                        u64 wondering_food_search_cycles = __rdtsc();
                        LinkedList* food_cell = &game_state->food_cells[y][x];
                        for(LinkedList* node=food_cell->next; node != food_cell; node=node->next){
                            food = node->data;
                            if(!food->food_targeted){
                                f32 distance = distance2(ant->position, food->position);
                                if(distance < 20){
                                    //ant->color = ORANGE;
                                    ant->ant_state = AntState_Collecting;
                                    food->food_targeted = true;
                                    ant->ant_food = food;
                                    food->food_ant = ant;

                                    ant->target_direction = direction(ant->position, food->position);
                                    ant->rot_percent = 0.0f;
                                    game_state->wondering_food_search_cycles += get_cpu_cycles_elapsed(__rdtsc(), wondering_food_search_cycles);
                                    goto end_of_behavior;
                                }
                            }
                        }
                        game_state->wondering_food_search_cycles += get_cpu_cycles_elapsed(__rdtsc(), wondering_food_search_cycles);

                        u64 wondering_pher_search_cycles = __rdtsc();
                        LinkedList* pher_cell = &game_state->pher_food_cells[y][x];
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
                        game_state->wondering_pher_search_cycles += get_cpu_cycles_elapsed(__rdtsc(), wondering_pher_search_cycles);
                    }
                }
            }
            game_state->wondering_search_cycles += get_cpu_cycles_elapsed(__rdtsc(), wondering_search_cycles);

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
            game_state->wondering_cycles += get_cpu_cycles_elapsed(__rdtsc(), wondering_cycles);
        }

        else if(ant->ant_state == AntState_Collecting){
            u64 collecting_cycles = __rdtsc();

            ant->target_direction = direction(ant->position, ant->ant_food->position);
            f32 distance = distance2(ant->position, ant->ant_food->position);

            // consume food once close enough
            if(distance < 2){
                ant->ant_state = AntState_Depositing;
                ant->ant_food->food_collected = true;
                ant->target_direction.x = -ant->direction.x;
                ant->target_direction.y = -ant->direction.y;
                ant->rot_percent = 0.0f;
                ant->direction_change_timer_max = (random_range(3) + 1);
                ant->direction_change_timer = clock->get_ticks();
                ant->color = RED;
            }
            game_state->collecting_cycles += get_cpu_cycles_elapsed(__rdtsc(), collecting_cycles);
        }

        else if(ant->ant_state == AntState_Depositing){
            u64 depositing_cycles = __rdtsc();

            if(!ant->colony_targeted){
                u64 depositing_search_cycles = __rdtsc();

                Entity *pher = NULL;
                for(i32 y=bottom_left_cell_coord.y; y<=center_cell_coord.y+1; ++y){
                    for(i32 x=bottom_left_cell_coord.x; x<=center_cell_coord.x+1; ++x){
                        if(x >= 0 && x < game_state->cell_row_count && y >= 0 && y < game_state->cell_row_count){
                            LinkedList* pher_cell = &game_state->pher_home_cells[y][x];
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
                game_state->depositing_search_cycles += get_cpu_cycles_elapsed(__rdtsc(), depositing_search_cycles);

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
            f32 colony_distance = distance2(ant->position, game_state->colony->position);
            if(colony_distance < 75){
                ant->target_direction = direction2(ant->position, game_state->colony->position);
                if(!ant->colony_targeted){
                    ant->rot_percent = 0;
                    ant->colony_targeted = true;
                }
            }

            // deposite if close to colony
            if(colony_distance < 2){
                game_state->free_entities_at++;
                game_state->free_entities[game_state->free_entities_at] = ant->ant_food->index;
                ant->ant_food->type = EntityType_None;

                ant->ant_state = AntState_Wondering;
                ant->target_direction.x = -ant->direction.x;
                ant->target_direction.y = -ant->direction.y;
                ant->colony_targeted = false;
                ant->rot_percent = 0.0f;
                ant->direction_change_timer_max = (random_range(3) + 1);
                ant->direction_change_timer = clock->get_ticks();
                ant->ant_food->food_targeted = false;
                ant->ant_food->food_collected = false;
                ant->ant_food->food_ant = NULL;
                ant->ant_food = NULL;
                ant->color = LGRAY;
            }
            game_state->depositing_cycles += get_cpu_cycles_elapsed(__rdtsc(), depositing_cycles);
        }
        end_of_behavior:;
    }

#if RDTSC
    f32 ant_behavior = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);

    game_state->frame_count++;
    game_state->frame_average += frame_cycles;
    f32 frame_cycle_average = game_state->frame_average / game_state->frame_count;
    print("frame_cycles:               %0.05f\n", frame_cycles);
    print("reset_sentinels_cycles:     %0.05f\n", reset_sentinel_cycles);
    print("setup_sentinels_cycles:     %0.05f\n", LL_setup);
    print("events_cycles:              %0.05f\n", events_cycles);
    print("controller_cycles:          %0.05f\n", controller_cycles);
    print("pher_degredate_cycles:      %0.05f\n", pher_degredation);
    print("ant_behavior_cycles:        %0.05f\n", ant_behavior);
    print("     wondering_cycles:      %0.05f\n", game_state->wondering_cycles);
    print("         food+pher_cycles:  %0.05f\n", game_state->wondering_search_cycles);
    print("         food_cycles:       %0.05f\n", game_state->wondering_food_search_cycles);
    print("         pher_home_cycles:  %0.05f\n", game_state->wondering_pher_search_cycles);
    print("     collecting_cycles:     %0.05f\n", game_state->collecting_cycles);
    print("     depositing_cycles:     %0.05f\n", game_state->depositing_cycles);
    print("         pher_food_cycles:  %0.05f\n", game_state->depositing_search_cycles);
    game_state->wondering_cycles = 0;
    game_state->wondering_search_cycles = 0;
    game_state->wondering_food_search_cycles = 0;
    game_state->wondering_pher_search_cycles = 0;
    game_state->collecting_cycles = 0;
    game_state->depositing_cycles = 0;
    game_state->depositing_search_cycles = 0;

    cpu_start = __rdtsc();
#endif

    transient_state->render_commands_buffer->used_bytes = 0;
    allocate_clear_color(transient_state->render_commands_buffer, BLACK);
    for(u32 entity_index = 0; entity_index <= game_state->entities_size; ++entity_index){
        Entity *e = game_state->entities + entity_index;
        switch(e->type){
            case EntityType_Player:{
                //allocate_bitmap(transient_state->render_commands_buffer, e->position, e->image);
                //if(e->draw_bounding_box){
                //    allocate_box(transient_state->render_commands_buffer, e->position, e->dimension, e->color);
                //}
                //allocate_rect(transient_state->render_commands_buffer, e->position, e->dimension, e->color);
                allocate_box(transient_state->render_commands_buffer, e->position, e->dimension, e->color);
            }break;
            case EntityType_Pixel:{
                allocate_pixel(transient_state->render_commands_buffer, e->position, e->color);
            }break;
            case EntityType_Segment:{
                allocate_segment(transient_state->render_commands_buffer, e->p0, e->p1, e->color);
            }break;
            case EntityType_Line:{
                allocate_line(transient_state->render_commands_buffer, e->position, e->direction, e->color);
            }break;
            case EntityType_Ray:{
                allocate_ray(transient_state->render_commands_buffer, e->position, e->direction, e->color);
            }break;
            case EntityType_Rect:{
                allocate_rect(transient_state->render_commands_buffer, e->position, e->dimension, e->color);
            }break;
            case EntityType_Box:{
                allocate_box(transient_state->render_commands_buffer, e->position, e->dimension, e->color);
            }break;
            case EntityType_Quad:{
                allocate_quad(transient_state->render_commands_buffer, e->p0, e->p1, e->p2, e->p3, e->color, e->fill);
            }break;
            case EntityType_Triangle:{
                allocate_triangle(transient_state->render_commands_buffer, e->p0, e->p1, e->p2, e->color, e->fill);
            }break;
            case EntityType_Circle:{
                allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, e->fill);
                v2 dimenstion = vec2(e->rad*2, e->rad*2);
                v2 position = vec2(e->position.x - e->rad, e->position.y - e->rad);
                if(e->draw_bounding_box){
                    allocate_box(transient_state->render_commands_buffer, position, dimenstion, e->color);
                }
            }break;
            case EntityType_Bitmap:{
                allocate_bitmap(transient_state->render_commands_buffer, e->position, e->image);
            }break;
            case EntityType_None:{
            }break;
            case EntityType_Object:{
            }break;
            case EntityType_Food:{
                allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, e->fill);
            }break;
            case EntityType_Ant:{
                //HERE
                // TODO: segments seem to crash the game, figure out why
                //allocate_segment(transient_state->render_commands_buffer, e->position, game_state->controller.mouse_pos, RED);

				if(game_state->draw_wondering_ants && equal4(e->color, LGRAY)){
					allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, e->fill);
				}
                if(game_state->draw_depositing_ants && equal4(e->color, RED)){
					allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, e->fill);
				}
                if(equal4(e->color, ORANGE)){
					allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, e->fill);
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

                //allocate_box(transient_state->render_commands_buffer, right_rect.position, right_rect.dimension, YELLOW);
                //allocate_box(transient_state->render_commands_buffer, mid_rect.position, mid_rect.dimension, GREEN);
                //allocate_box(transient_state->render_commands_buffer, left_rect.position, left_rect.dimension, ORANGE);

                //allocate_circle(transient_state->render_commands_buffer, e->right_sensor, e->sensor_radius, YELLOW, false);
                //allocate_circle(transient_state->render_commands_buffer, e->mid_sensor, e->sensor_radius, GREEN, false);
                //allocate_circle(transient_state->render_commands_buffer, e->left_sensor, e->sensor_radius, ORANGE, false);

                //allocate_segment(transient_state->render_commands_buffer, e->position, e->right_sensor, YELLOW);
                //allocate_segment(transient_state->render_commands_buffer, e->position, e->mid_sensor, GREEN);
                //allocate_segment(transient_state->render_commands_buffer, e->position, e->left_sensor, ORANGE);
                //allocate_ray(transient_state->render_commands_buffer, e->position, e->target_direction, BLUE);
            }break;
            case EntityType_Colony:{
                allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, e->fill);
            }break;
            case EntityType_ToHomePheromone:{
                if(game_state->draw_home_phers){
                    allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, true);
                }
            }break;
            case EntityType_ToFoodPheromone:{
                if(game_state->draw_food_phers){
                    allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, true);
                }
            }break;
        }
    }
#if RDTSC
    f32 push_draw_commands = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);
    print("allocate_commands_cycles:   %0.05f\n", push_draw_commands);
    cpu_start = __rdtsc();
#endif
    draw_commands(render_buffer, transient_state->render_commands_buffer);
#if RDTSC
    f32 draw_cycles = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);
    game_state->draw_count++;
    game_state->draw_average += draw_cycles;
    f32 draw_cycles_average = game_state->draw_average / game_state->draw_count;
    print("draw_cycles:            %0.05f\n", draw_cycles);
#endif
    game_state->frame_index++;
}
