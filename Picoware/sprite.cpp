#include "../../../../internal/applications/games/freeroam/sprite.hpp"
#include <math.h>

Sprite::Sprite(Board board, const char *name, Vector position, Sprite3DType spriteType,
               float height, float width, float rotation, Vector endPosition)
    : Entity(board, name, ENTITY_3D_SPRITE, position, Vector(10, 10), nullptr,
             nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, false, false,
             spriteType)
{
    set3DSpriteRotation(rotation);
    start_position = position;
    end_position = endPosition == Vector(-1, -1) ? position : endPosition;
    state = ENTITY_MOVING_TO_END;
}

void Sprite::collision(Entity *other, Game *game)
{
    if (other->type != ENTITY_PLAYER)
    {
        return;
    }
    // I'll come back to this...
    // FURI_LOG_I("Sprite", "Collision with player detected: %s", other->name);
    // this->position_set(this->old_position);
}

void Sprite::update(Game *game)
{
    // Only move if start and end positions are different
    if (start_position.x == end_position.x && start_position.y == end_position.y)
    {
        return; // No movement needed
    }

    const float DISTANCE_THRESHOLD = 0.2f; // Minimum distance to consider "reached"
    const float MOVE_SPEED = 0.1f;         // Even faster movement speed

    // move towards the end position if the state is MOVING_TO_END
    if (state == ENTITY_MOVING_TO_END)
    {
        float dx = end_position.x - position.x;
        float dy = end_position.y - position.y;
        float distance = sqrtf(dx * dx + dy * dy);

        if (distance > DISTANCE_THRESHOLD)
        {
            // Normalize direction and apply speed
            float move_x = (dx / distance) * MOVE_SPEED;
            float move_y = (dy / distance) * MOVE_SPEED;

            // Don't overshoot the target
            if (distance < MOVE_SPEED)
            {
                move_x = dx;
                move_y = dy;
            }

            position_set(Vector(position.x + move_x, position.y + move_y));
            position_changed = true;
        }
        else
        {
            // Snap to exact position when close enough
            position_set(end_position);
            position_changed = true;
            state = ENTITY_MOVING_TO_START;
        }
    }
    else if (state == ENTITY_MOVING_TO_START)
    {
        float dx = start_position.x - position.x;
        float dy = start_position.y - position.y;
        float distance = sqrtf(dx * dx + dy * dy);

        if (distance > DISTANCE_THRESHOLD)
        {
            // Normalize direction and apply speed
            float move_x = (dx / distance) * MOVE_SPEED;
            float move_y = (dy / distance) * MOVE_SPEED;

            // Don't overshoot the target
            if (distance < MOVE_SPEED)
            {
                move_x = dx;
                move_y = dy;
            }

            position_set(Vector(position.x + move_x, position.y + move_y));
            position_changed = true;
        }
        else
        {
            // Snap to exact position when close enough
            position_set(start_position);
            position_changed = true;
            state = ENTITY_MOVING_TO_END;
        }
    }
}