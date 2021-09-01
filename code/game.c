#include "game.h"

#define RDTSC 0
// ant logic can be simplified dramatically

static v4
get_color_at(RenderBuffer *buffer, f32 x, f32 y){
    v4 result = {0};

    u8 *location = (u8 *)buffer->memory +
                    ((buffer->height - (i32)y - 1) * buffer->pitch) +
                    ((i32)x * buffer->bytes_per_pixel);
    u32 *pixel = (u32 *)location;
    result.a = (f32)((*pixel >> 24) & 0xFF) / 255.0f;
    result.r = (f32)((*pixel >> 16) & 0xFF) / 255.0f;
    result.g = (f32)((*pixel >> 8) & 0xFF) / 255.0f;
    result.b = (f32)((*pixel >> 0) & 0xFF) / 255.0f;

    return(result);
}

static void
copy_array(v2 *a, v2 *b, i32 count){
    for(i32 i=0; i < count; ++i){
        *a++ = *b++;
    }
}


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
    clock->cpu_now = __rdtsc();
    f32 cpu_cycles = (f32)(clock->cpu_now - clock->cpu_prev) / (1000 * 1000);
    f32 c_cycles = get_cpu_cycles_elapsed(clock->cpu_prev, clock->cpu_now);
    clock->cpu_prev = clock->cpu_now;

#if RDTSC
    print("%0.05f - %0.05f\n", cpu_cycles, c_cycles);
#endif

    v4 RED =     {1.0f, 0.0f, 0.0f,  0.5f};
    v4 GREEN =   {0.0f, 1.0f, 0.0f,  0.5f};
    v4 BLUE =    {0.0f, 0.0f, 1.0f,  0.5f};
    v4 MAGENTA = {1.0f, 0.0f, 1.0f,  1.0f};
    v4 PINK =    {0.92f, 0.62f, 0.96f, 0.5f};
    v4 YELLOW =  {0.9f, 0.9f, 0.0f,  0.5f};
    v4 TEAL =    {0.0f, 1.0f, 1.0f,  0.5f};
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
                                        Megabytes(1000));


        game_state->screen_width = render_buffer->width;
        game_state->screen_height = render_buffer->height;
        game_state->ant_speed = 40.0f;

        seed_random(time(NULL), 0);

        game_state->cell_row_count = 16;
        game_state->cell_width = render_buffer->width / game_state->cell_row_count;
        game_state->cell_height = render_buffer->height / game_state->cell_row_count;
        //game_state->ants_count = 3000;
        game_state->ants_count = 1000;
        //game_state->ants_count = 100;
        //game_state->ants_count = 10;
        //game_state->ants_count = 1;
        game_state->entities_size = 110000;
        game_state->free_entities_size = 110000;
        //game_state->entities_size = 110;
        //game_state->free_entities_size = 110;
        game_state->free_entities_at = game_state->free_entities_size - 1;

        u32 none_index = add_entity(game_state, EntityType_None);

        game_state->test = load_bitmap(memory, "test.bmp");
        game_state->circle = load_bitmap(memory, "circle.bmp");
        game_state->image = load_bitmap(memory, "image.bmp");

        // setup free entities array
        for(i32 i = game_state->free_entities_size - 1; i >= 0; --i){
            game_state->free_entities[i] = game_state->free_entities_size - 1 - i;
        }

        for(u32 i=0; i < game_state->ants_count; ++i){
            u32 index = add_ant(game_state, vec2(render_buffer->width/2, render_buffer->height/2), 2, LGRAY, true);
            Entity *ant = game_state->entities + index;
            ant->direction_change_timer = clock->get_ticks();
            ant->pheromone_spawn_timer = clock->get_ticks();
        }


        //for(i32 i=0; i<game_state->cell_row_count; ++i){
        //    add_segment(game_state, vec2(0, render_buffer->height - game_state->cell_height * i), vec2(render_buffer->width, render_buffer->height - game_state->cell_height * i), DGRAY);
        //    add_segment(game_state, vec2(render_buffer->width - game_state->cell_width * i, 0), vec2(render_buffer->width - game_state->cell_width * i, render_buffer->height), DGRAY);
        //}
        //game_state->colony_index = add_colony(game_state, vec2(render_buffer->width/2, render_buffer->height/2), 25, DGRAY, true);

        game_state->add_food = false;
        memory->initialized = true;
        f32 cycles = get_cpu_cycles_elapsed(__rdtsc(), init_cpu_length);
#if RDTSC
        print("init_cycles: %0.0f\n", cycles);
    }

    print("-----------------------------------------\n");
#else
    }
#endif

    u64 cpu_start = __rdtsc();
    reset_LL_sentinel(&game_state->ants);
    reset_LL_sentinel(&game_state->misc);
    for(i32 j=0; j < game_state->cell_row_count; ++j){
        for(i32 k=0; k < game_state->cell_row_count; ++k){
            reset_LL_sentinel(&game_state->pher_cells[j][k]);
            reset_LL_sentinel(&game_state->food_pher_cells[j][k]);
            reset_LL_sentinel(&game_state->home_pher_cells[j][k]);
            reset_LL_sentinel(&game_state->food_cells[j][k]);
        }
    }
    transient_state->LL_buffer->used_bytes = 0;
    f32 reset_sentinel_cycles = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);
#if RDTSC
    print("reset_sentinels_cycles: %0.05f\n", reset_sentinel_cycles);
#endif

    cpu_start = __rdtsc();
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
            }break;
            case EntityType_Food:{
                v2 cell_coord = vec2(floor(e->position.x / game_state->cell_width), floor(e->position.y / game_state->cell_height));
                cell_coord.x = clamp_f32(0, cell_coord.x, game_state->cell_row_count - 1);
                cell_coord.y = clamp_f32(0, cell_coord.y, game_state->cell_row_count - 1);
                LinkedList* cell = &game_state->food_cells[(i32)cell_coord.y][(i32)cell_coord.x];
                LinkedList* node = allocate_LL_node(transient_state->LL_buffer);
                node->data = e;
                LL_push(cell, node);
            }break;
            case EntityType_ToHomePheromone:{
                v2 cell_coord = vec2(floor(e->position.x / game_state->cell_width), floor(e->position.y / game_state->cell_height));
                cell_coord.x = clamp_f32(0, cell_coord.x, game_state->cell_row_count - 1);
                cell_coord.y = clamp_f32(0, cell_coord.y, game_state->cell_row_count - 1);

                LinkedList* cell = &game_state->home_pher_cells[(i32)cell_coord.y][(i32)cell_coord.x];
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
                LinkedList* cell = &game_state->food_pher_cells[(i32)cell_coord.y][(i32)cell_coord.x];
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
    for(LinkedList* node=game_state->misc.next; node != &game_state->misc; node=node->next){
        Entity *e = node->data;
        e->color = DGRAY;
    }
    f32 LL_setup = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);
#if RDTSC
    print("setup_sentinels_cycles: %0.05f\n", LL_setup);
#endif

    // keyboard/controller/mouse events
    cpu_start = __rdtsc();
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
            if(event->key == KEY_W){
                game_state->controller.up = true;
            }
            if(event->key == KEY_S){
                game_state->controller.down = true;
            }
            if(event->key == KEY_A){
                game_state->controller.left = true;
            }
            if(event->key == KEY_D){
                game_state->controller.right = true;
            }
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
    f32 events_cycles = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);
#if RDTSC
    print("events_cycles:          %0.05f\n", events_cycles);
#endif

    if(game_state->controller.m1){
        u32 index = add_to_food_pheromone(game_state, game_state->controller.mouse_pos, 1, TEAL);
        Entity *pher = game_state->entities + index;
        pher->food_decay_timer = clock->get_ticks();
    }
    if(game_state->controller.m2){
        u32 index = add_to_home_pheromone(game_state, game_state->controller.mouse_pos, 1, MAGENTA);
        Entity *pher = game_state->entities + index;
        pher->home_decay_timer = clock->get_ticks();
    }
    if(game_state->controller.m3){
        //game_state->fc++;
        if(!game_state->add_food){
            game_state->add_food = true;
            v2 cell_coord = vec2(floor(game_state->controller.mouse_pos.x / game_state->cell_width), floor(game_state->controller.mouse_pos.y / game_state->cell_height));
            cell_coord.x = clamp_f32(0, cell_coord.x, game_state->cell_row_count - 1);
            cell_coord.y = clamp_f32(0, cell_coord.y, game_state->cell_row_count - 1);
            i32 x_start = cell_coord.x * game_state->cell_width;
            i32 y_start = cell_coord.y * game_state->cell_height;
            for(i32 x = x_start; x < (i32)(x_start + game_state->cell_width); x+=5){
                for(i32 y = y_start; y < (i32)(y_start + game_state->cell_height); y+=5){
                    add_food(game_state, vec2(x, y), 2, GREEN, true);
                    game_state->ccc++;
                }
            }
        }
    }
    else{
        game_state->add_food = false;
    }

    cpu_start = __rdtsc();
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
                        e->color.a = lerp(1.0f, 0.0f, t);
                        if(e->color.a <= 0){
                            e->color.a = 1.0f;
                            e->type = EntityType_None;
                            game_state->free_entities_at++;
                            game_state->free_entities[game_state->free_entities_at] = e->index;
                        }
                    }break;
                    case EntityType_ToHomePheromone:{
                        u64 now = clock->get_ticks();
                        f32 seconds_elapsed = clock->get_seconds_elapsed(e->home_decay_timer, now);
                        f32 t = seconds_elapsed / e->pher_home_decay_rate;
                        e->color.a = lerp(1.0f, 0.0f, t);

                        if(e->color.a <= 0.0f){
                            e->color.a = 1.0f;
                            e->type = EntityType_None;
                            game_state->free_entities_at++;
                            game_state->free_entities[game_state->free_entities_at] = e->index;
                        }
                    }break;
                }
            }
        }
    }
    f32 pher_degredation = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);
#if RDTSC
    print("pher_degredate_cycles:  %0.05f\n", pher_degredation);
#endif

    cpu_start = __rdtsc();
    for(LinkedList* node=game_state->ants.next; node != &game_state->ants; node=node->next){
        Entity *ant = (Entity*)node->data;

        u64 c3 = __rdtsc();
        if(ant->ant_state == AntState_Wondering){
            ant->color = LGRAY;
            //game_state->wc++;
            ant->right_sensor_density = 0.0f;
            ant->middle_sensor_density = 0.0f;
            ant->left_sensor_density = 0.0f;

            if(ant->changing_state){
                ant->rot_percent = 0.0f;
                ant->changing_state = false;
            }

            v2 middle_coord = vec2(floor(ant->position.x / game_state->cell_width), floor(ant->position.y / game_state->cell_height));
            middle_coord.x = clamp_f32(0, middle_coord.x, game_state->cell_row_count - 1);
            middle_coord.y = clamp_f32(0, middle_coord.y, game_state->cell_row_count - 1);
            v2 bottom_left = vec2(middle_coord.x - 1, middle_coord.y - 1);
            u64 c1 = __rdtsc();
            for(i32 y=bottom_left.y; y<=middle_coord.y+1; ++y){
                for(i32 x=bottom_left.x; x<=middle_coord.x+1; ++x){
                    if(x >= 0 && x < game_state->cell_row_count && y >= 0 && y < game_state->cell_row_count){
                        //add_box(game_state, vec2(game_state->cell_width * x, game_state->cell_height * y), vec2(game_state->cell_width, game_state->cell_height), ORANGE);
                        LinkedList* food_cell = &game_state->food_cells[y][x];
                        Rect right_rect = rect(vec2(ant->right_sensor.x - ant->sensor_radius, ant->right_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                        Rect mid_rect = rect(vec2(ant->mid_sensor.x - ant->sensor_radius, ant->mid_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                        Rect left_rect = rect(vec2(ant->left_sensor.x - ant->sensor_radius, ant->left_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
                        Entity *food = NULL;
                        u64 c0 = __rdtsc();
                        for(LinkedList* node=food_cell->next; node != food_cell; node=node->next){
                            food = node->data;
                            if(!food->targeted){
                                if(rect_collide_point(right_rect, food->position) || rect_collide_point(mid_rect, food->position) || rect_collide_point(left_rect, food->position)){
                                    ant->ant_state = AntState_Collecting;
                                    ant->ant_food = food;
                                    food->targeted = true;
                                    ant->changing_state = true;
                                    break;
                                }
                            }
                        }
                        game_state->c0_total += get_cpu_cycles_elapsed(__rdtsc(), c0);
                        u64 c2 = __rdtsc();
                        // TODO: not measuring this yet
                        LinkedList* pher_cell = &game_state->food_pher_cells[(i32)middle_coord.y][(i32)middle_coord.x];
                        Entity *pher = NULL;
                        for(LinkedList *node=pher_cell->next; node != pher_cell; node=node->next){
                            pher = node->data;
                            if(rect_collide_point(right_rect, pher->position)){
                                ant->right_sensor_density += pher->color.a;
                            }
                            if(rect_collide_point(mid_rect, pher->position)){
                                ant->middle_sensor_density += pher->color.a;
                            }
                            if(rect_collide_point(left_rect, pher->position)){
                                ant->left_sensor_density += pher->color.a;
                            }
                        }
                        f32 result2 = get_cpu_cycles_elapsed(__rdtsc(), c2);
                        game_state->c2_total += result2;
                    }
                }
            }
            f32 result1 = get_cpu_cycles_elapsed(__rdtsc(), c1);
            game_state->c1_total += result1;

            if(ant->ant_state != AntState_Collecting){
                if(ant->right_sensor_density > 0 || ant->middle_sensor_density > 0 || ant->left_sensor_density > 0){
                    //if(ant->rot_percent == 1){
                        //print("ok\n");
                        f32 forward_rad = dir_rad(ant->direction);
                        f32 right_rad = forward_rad + (RAD * ant->sensor_angle);
                        f32 left_rad = forward_rad - (RAD * ant->sensor_angle);

                        v2 right_direction = rad_dir(right_rad);
                        v2 left_direction = rad_dir(left_rad);
                        if(ant->middle_sensor_density > MAXf32(ant->right_sensor_density, ant->left_sensor_density)){
                            ant->target_direction = ant->direction;
                            ant->rot_percent = 0.0f;
                        }
                        else if(ant->right_sensor_density > ant->left_sensor_density){
                            ant->target_direction = right_direction;
                            ant->rot_percent = 0.0f;
                        }
                        else if(ant->left_sensor_density > ant->right_sensor_density){
                            ant->target_direction = left_direction;
                            ant->rot_percent = 0.0f;
                        }
                    //}
                }
                else{
                    // random target_direction on timer
                    u64 now = clock->get_ticks();
                    f32 seconds_elapsed = clock->get_seconds_elapsed(ant->direction_change_timer, now);
                    if(seconds_elapsed >= ant->direction_change_timer_max){
                        ant->random_vector.x = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                        ant->random_vector.y = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                        ant->rot_percent = 0.0f;
                        ant->direction_change_timer_max = (random_range(3) + 1);
                        ant->direction_change_timer = clock->get_ticks();
                        ant->target_direction = ant->random_vector;
                    }
                }

                // rotate towards target_direction
                f32 ant_angle = dir_rad(ant->direction);
                f32 target_angle = dir_rad(ant->target_direction);
                ant->direction = rad_dir(lerp_rad(ant_angle, target_angle, ant->rot_percent));
                ant->rot_percent += ant->rotate_speed;
                if(ant->rot_percent > 1.0f){
                    ant->rot_percent = 1.0f;
                }

                // move towards ants forward direction
                if((ant->position.x + (game_state->ant_speed * ant->direction.x) * clock->dt) < 0.0f ||
                   (ant->position.x + (game_state->ant_speed * ant->direction.x) * clock->dt) > render_buffer->width){
                    ant->direction.x = -ant->direction.x;
                    ant->random_vector.x = -ant->random_vector.x;
                    ant->target_direction.x = -ant->target_direction.x;
                }
                else if((ant->position.y + (game_state->ant_speed * ant->direction.y) * clock->dt) < 0 ||
                        (ant->position.y + (game_state->ant_speed * ant->direction.y) * clock->dt) > render_buffer->height){
                    ant->direction.y = -ant->direction.y;
                    ant->random_vector.y = -ant->random_vector.y;
                    ant->target_direction.y = -ant->target_direction.y;
                }
                else{
                    ant->position.x += (game_state->ant_speed * ant->direction.x) * clock->dt;
                    ant->position.y += (game_state->ant_speed * ant->direction.y) * clock->dt;
                }

                // to home pheromones
                u64 now = clock->get_ticks();
                f32 seconds_elapsed = clock->get_seconds_elapsed(ant->pheromone_spawn_timer, now);
                if(seconds_elapsed >= ant->pheromone_spawn_timer_max){
                    ant->pheromone_spawn_timer = clock->get_ticks();
                    u32 index = add_to_home_pheromone(game_state, ant->position, 1, MAGENTA);
                    Entity *pher = game_state->entities + index;
                    pher->home_decay_timer = clock->get_ticks();
                }

            }
        }
        f32 result3 = get_cpu_cycles_elapsed(__rdtsc(), c3);
        game_state->c3_total += result3;
        u64 c4 = __rdtsc();
        if(ant->ant_state == AntState_Collecting){
            if(ant->changing_state){
                ant->rot_percent = 0.0f;
                ant->changing_state = false;
            }

            // rotate towards target_direction (food)
            ant->target_direction = direction2(ant->position, ant->ant_food->position);
            f32 ant_angle = dir_rad(ant->direction);
            f32 target_angle = dir_rad(ant->target_direction);
            ant->direction = rad_dir(lerp_rad(ant_angle, target_angle, ant->rot_percent));
            ant->rot_percent += ant->rotate_speed;
            if(ant->rot_percent > 1.0f){
                ant->rot_percent = 1.0f;
            }

            // move towards ants forward direction
            ant->position.x += (game_state->ant_speed * ant->direction.x) * clock->dt;
            ant->position.y += (game_state->ant_speed * ant->direction.y) * clock->dt;

            // consume food once close enough
            f32 distance = distance2(ant->position, ant->ant_food->position);
            if(distance < 10){
                //game_state->fc--;
                ant->ant_state = AntState_Depositing;
                ant->ant_food->targeted = false;
                ant->target_direction.x = -ant->target_direction.x;
                ant->target_direction.y = -ant->target_direction.y;

                ant->ant_food->type = EntityType_None;
                game_state->free_entities_at++;
                game_state->free_entities[game_state->free_entities_at] = ant->ant_food->index;
                ant->changing_state = true;
            }
        }
        f32 result4 = get_cpu_cycles_elapsed(__rdtsc(), c4);
        game_state->c4_total += result4;
        u64 c5 = __rdtsc();
        if(ant->ant_state == AntState_Depositing){
            //game_state->dc++;
            ant->color = RED;
            ant->right_sensor_density = 0.0f;
            ant->middle_sensor_density = 0.0f;
            ant->left_sensor_density = 0.0f;

            if(ant->changing_state){
                ant->rot_percent = 0.0f;
                ant->changing_state = false;
            }

            u64 c6 = __rdtsc();
            v2 middle_coord = vec2(floor(ant->position.x / game_state->cell_width), floor(ant->position.y / game_state->cell_height));
            middle_coord.x = clamp_f32(0, middle_coord.x, game_state->cell_row_count - 1);
            middle_coord.y = clamp_f32(0, middle_coord.y, game_state->cell_row_count - 1);
            v2 bottom_left = vec2(middle_coord.x - 1, middle_coord.y - 1);
            Rect right_rect = rect(vec2(ant->right_sensor.x - ant->sensor_radius, ant->right_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
            Rect mid_rect = rect(vec2(ant->mid_sensor.x - ant->sensor_radius, ant->mid_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
            Rect left_rect = rect(vec2(ant->left_sensor.x - ant->sensor_radius, ant->left_sensor.y - ant->sensor_radius), vec2(ant->sensor_radius * 2, ant->sensor_radius * 2));
            Entity *pher = NULL;
            for(i32 y=bottom_left.y; y<=middle_coord.y+1; ++y){
                for(i32 x=bottom_left.x; x<=middle_coord.x+1; ++x){
                    if(x >= 0 && x < game_state->cell_row_count && y >= 0 && y < game_state->cell_row_count){
                        LinkedList* pher_cell = &game_state->home_pher_cells[y][x];
                        for(LinkedList* node=pher_cell->next; node != pher_cell; node=node->next){
                            pher = node->data;
                            if(rect_collide_point(right_rect, pher->position)){
                                ant->right_sensor_density += pher->color.a;
                            }
                            if(rect_collide_point(mid_rect, pher->position)){
                                ant->middle_sensor_density += pher->color.a;
                            }
                            if(rect_collide_point(left_rect, pher->position)){
                                ant->left_sensor_density += pher->color.a;
                            }
                        }
                    }
                }
            }
            f32 result6 = get_cpu_cycles_elapsed(__rdtsc(), c6);
            game_state->c6_total += result6;

            if(ant->right_sensor_density > 0 || ant->middle_sensor_density > 0 || ant->left_sensor_density > 0){
                f32 forward_rad = dir_rad(ant->direction);
                f32 right_rad = forward_rad + (RAD * ant->sensor_angle);
                f32 left_rad = forward_rad - (RAD * ant->sensor_angle);

                v2 right_direction = rad_dir(right_rad);
                v2 left_direction = rad_dir(left_rad);
                if(ant->middle_sensor_density > MAXf32(ant->right_sensor_density, ant->left_sensor_density)){
                    ant->target_direction = ant->direction;
                    ant->rot_percent = 0.0f;
                }
                else if(ant->right_sensor_density > ant->left_sensor_density){
                    ant->target_direction = right_direction;
                    ant->rot_percent = 0.0f;
                }
                else if(ant->left_sensor_density > ant->right_sensor_density){
                    ant->target_direction = left_direction;
                    ant->rot_percent = 0.0f;
                }
            }
            else{
                // direction change timer
                u64 now = clock->get_ticks();
                f32 seconds_elapsed = clock->get_seconds_elapsed(ant->direction_change_timer, now);
                if(seconds_elapsed >= ant->direction_change_timer_max){
                    ant->random_vector.x = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                    ant->random_vector.y = (((i32)(random_range((2 * 100) + 1)) - 100)/100.0f);
                    ant->rot_percent = 0.0f;
                    ant->direction_change_timer_max = (random_range(3) + 1);
                    ant->direction_change_timer = clock->get_ticks();
                    ant->target_direction = ant->random_vector;
                }
            }

            Entity* colony = game_state->entities + game_state->colony_index;
            f32 colony_distance = distance2(ant->position, colony->position);
            if(colony_distance < 100){
                if(!ant->colony_targeted){
                    ant->target_direction = direction2(ant->position, colony->position);
                    ant->rot_percent = 0;
                    ant->colony_targeted = true;
                }
            }

            // rotate towards target_angle
            f32 ant_angle = dir_rad(ant->direction);
            f32 target_angle = dir_rad(ant->target_direction);
            ant->direction = rad_dir(lerp_rad(ant_angle, target_angle, ant->rot_percent));
            ant->rot_percent += ant->rotate_speed;
            if(ant->rot_percent > 1.0f){
                ant->rot_percent = 1.0f;
            }

            // move forward direction
            ant->position.x += (game_state->ant_speed * ant->direction.x) * clock->dt;
            ant->position.y += (game_state->ant_speed * ant->direction.y) * clock->dt;

            // deposite if close to colony
            //Entity* colony = game_state->entities + game_state->colony_index;
            //f32 colony_distance = distance2(ant->position, colony->position);
            if(colony_distance < 20){
                ant->ant_state = AntState_Wondering;
                ant->changing_state = true;
                ant->ant_food = NULL;
                ant->target_direction.x = -ant->target_direction.x;
                ant->target_direction.y = -ant->target_direction.y;
                ant->colony_targeted = false;
            }

            // to food pheromones
            u64 now = clock->get_ticks();
            f32 seconds_elapsed = clock->get_seconds_elapsed(ant->pheromone_spawn_timer, now);
            if(seconds_elapsed >= ant->pheromone_spawn_timer_max){
                ant->pheromone_spawn_timer = clock->get_ticks();
                u32 index = add_to_food_pheromone(game_state, ant->position, 1, TEAL);
                Entity *pher = game_state->entities + index;
                pher->food_decay_timer = clock->get_ticks();
            }
        }
        f32 result5 = get_cpu_cycles_elapsed(__rdtsc(), c5);
        game_state->c5_total += result5;
    }
    f32 ant_behavior = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);
#if RDTSC
    print("ant_behavior_cycles:    %0.05f\n", ant_behavior);
    print("     wondering_cycles:    %0.05f\n", game_state->c3_total);
    print("         food+pher_cycles:    %0.05f\n", game_state->c1_total);
    print("         food_cycles:    %0.05f\n", game_state->c0_total);
    print("         phers_cycles:    %0.05f\n", game_state->c2_total);
    print("     collecting_cycles:    %0.05f\n", game_state->c4_total);
    print("     depositing_cycles:    %0.05f\n", game_state->c5_total);
    print("         phers_cycles:    %0.05f\n", game_state->c6_total);
    game_state->c0_total = 0;
    game_state->c1_total = 0;
    game_state->c2_total = 0;
    game_state->c3_total = 0;
    game_state->c4_total = 0;
    game_state->c5_total = 0;
    game_state->c6_total = 0;
#endif

    cpu_start = __rdtsc();
    transient_state->render_commands_buffer->used_bytes = 0;
    allocate_clear_color(transient_state->render_commands_buffer, BLACK);
    for(u32 entity_index = 0; entity_index <= game_state->entities_size; ++entity_index){
        Entity *e = game_state->entities + entity_index;
        switch(e->type){
            case EntityType_Player:{
                allocate_bitmap(transient_state->render_commands_buffer, e->position, e->image);
                if(e->draw_bounding_box){
                    allocate_box(transient_state->render_commands_buffer, e->position, e->dimension, e->color);
                }
                //allocate_rect(transient_state->render_commands_buffer, e->position, e->dimension, e->color);
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


                allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, e->fill);

                f32 forward_dir = dir_rad(e->direction);
                f32 right_rad = forward_dir + (RAD * e->sensor_angle);
                f32 left_rad = forward_dir - (RAD * e->sensor_angle);

                v2 right_direction = rad_dir(right_rad);
                v2 left_direction = rad_dir(left_rad);

                e->right_sensor = vec2(e->position.x + (e->sensor_distance * right_direction.x), e->position.y + (e->sensor_distance * right_direction.y));
                e->mid_sensor = vec2(e->position.x + (e->sensor_distance * e->direction.x), e->position.y + (e->sensor_distance * e->direction.y));
                e->left_sensor = vec2(e->position.x + (e->sensor_distance * left_direction.x), e->position.y + (e->sensor_distance * left_direction.y));

                Rect right_rect = rect(vec2(e->right_sensor.x - e->sensor_radius, e->right_sensor.y - e->sensor_radius), vec2(e->sensor_radius * 2, e->sensor_radius * 2));
                Rect mid_rect = rect(vec2(e->mid_sensor.x - e->sensor_radius, e->mid_sensor.y - e->sensor_radius), vec2(e->sensor_radius * 2, e->sensor_radius * 2));
                Rect left_rect = rect(vec2(e->left_sensor.x - e->sensor_radius, e->left_sensor.y - e->sensor_radius), vec2(e->sensor_radius * 2, e->sensor_radius * 2));


                //allocate_box(transient_state->render_commands_buffer, vec2(e->right_sensor.x - e->sensor_radius, e->right_sensor.y - e->sensor_radius), vec2(e->sensor_radius * 2, e->sensor_radius * 2), YELLOW);
                //allocate_box(transient_state->render_commands_buffer, vec2(e->mid_sensor.x - e->sensor_radius, e->mid_sensor.y - e->sensor_radius), vec2(e->sensor_radius * 2, e->sensor_radius * 2), GREEN);
                //allocate_box(transient_state->render_commands_buffer, vec2(e->left_sensor.x - e->sensor_radius, e->left_sensor.y - e->sensor_radius), vec2(e->sensor_radius * 2, e->sensor_radius * 2), ORANGE);

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
                //allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, true);
            }break;
            case EntityType_ToFoodPheromone:{
                //allocate_circle(transient_state->render_commands_buffer, e->position, e->rad, e->color, true);
            }break;
        }
    }
    f32 push_draw_commands = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);
#if RDTSC
    print("draw_commands_cycles:   %0.05f\n", push_draw_commands);
#endif
    //print("DT: %.05f - X: %.02f - Y: %.02f\n", clock->dt, game_state->c2->position.x, game_state->c2->position.y);
    //print("fc: %i - wc: %i - cc: %i - dc: %i - tc: %i - ntc: %i - ptc: %i\n", game_state->fc, game_state->wc, game_state->cc, game_state->dc, game_state->tc, game_state->ntc, game_state->ptc);
    cpu_start = __rdtsc();
    draw_commands(render_buffer, transient_state->render_commands_buffer);
    f32 draw_cycles = get_cpu_cycles_elapsed(__rdtsc(), cpu_start);
#if RDTSC
    print("draw_cycles:            %0.05f\n", draw_cycles);
#endif
    game_state->wc = 0;
    game_state->cc = 0;
    game_state->dc = 0;
    game_state->tc = 0;
    game_state->ntc = 0;
    game_state->ptc = 0;
}
