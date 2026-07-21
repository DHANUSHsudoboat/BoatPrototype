# Unreal Engine Boat Movement System

Implement a realistic boat movement system in **Unreal Engine 5 (C++)**.

## Existing Class

The implementation should be added to the following class:

-   **Class Name:** `ABoat`
-   **Class Type:** `APawn`

Do **not** create a new movement component or a new Pawn class unless
absolutely necessary. Extend the existing `ABoat` class and keep the
code modular.

------------------------------------------------------------------------

# Gear System

The boat has **4 gear states**:

  Gear   Description
  ------ ----------------
  0      Idle / Neutral
  1      Slow Speed
  2      Cruise Speed
  3      Maximum Speed

## Input

### W Key

-   Pressing **W** shifts the boat up by one gear.
-   Gear progression:
    -   0 → 1
    -   1 → 2
    -   2 → 3
-   Gear should never exceed Gear 3.

### S Key

-   Pressing **S** shifts the boat down by one gear.
-   Gear progression:
    -   3 → 2
    -   2 → 1
    -   1 → 0
-   Gear should never go below Gear 0.

# Speed Behavior

Each gear has its own configurable target speed.

Example:

-   Gear 0 = 0
-   Gear 1 = Configurable
-   Gear 2 = Configurable
-   Gear 3 = Configurable

The exact values should be exposed as editable properties.

The boat should **never instantly jump** to the target speed.

Instead:

-   Shifting up should smoothly accelerate toward the new gear speed.
-   Shifting down should smoothly decelerate toward the lower gear
    speed.
-   Gear 0 should smoothly bring the boat to a complete stop.

Use acceleration and deceleration values instead of directly setting
velocity.

# Steering

Steering should feel like a real boat rather than a car.

The turning radius must depend on the current forward speed.

-   **Low Speed:** Small turning radius, sharp turns.
-   **High Speed:** Large turning radius, wide turns.

The steering response should scale smoothly using the current speed.

Avoid instant rotation, snapping, or car-like steering.

# Editable Variables

Expose these variables using `UPROPERTY(EditAnywhere)`:

-   Gear 1 Speed
-   Gear 2 Speed
-   Gear 3 Speed
-   Current Gear
-   Current Speed
-   Maximum Speed
-   Acceleration Rate
-   Deceleration Rate
-   Steering Sensitivity
-   Minimum Turn Radius
-   Maximum Turn Radius

# Code Structure

Suggested functions:

``` cpp
void ShiftUpGear();
void ShiftDownGear();

void UpdateSpeed(float DeltaTime);
void UpdateSteering(float DeltaTime);

float GetTargetSpeed() const;
float GetTurnMultiplier() const;
```

Separate gear management, speed interpolation, steering logic, and input
handling.

# Expected Gameplay

-   **W**: Shift 0 → 1 → 2 → 3 with smooth acceleration.
-   **S**: Shift 3 → 2 → 1 → 0 with smooth deceleration.
-   Steering changes dynamically with speed.
-   Boat movement should feel smooth, responsive, and have believable
    momentum.

# Important

-   Do **not** rewrite unrelated code.
-   Integrate the system into the existing `ABoat` (`APawn`) class.
-   Reuse existing input bindings if available.
-   Add any required variables or helper methods to `Boat.h` and
    `Boat.cpp`.
-   Produce complete, compilable Unreal Engine 5 C++ code.
