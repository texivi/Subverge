/* song data */

#include "audio.h"

#define CANON_A "l2 o5 f# e d c# <b a b >c# "
#define CANON_V "l8 o5 d f# a f# a o6 c# e c# o5 b o6 d f# d o5 f# a o6 c# o5 a g b o6 d o5 b d f# a f# g b o6 d o5 b a o6 c# e c# "
#define CANON_R "r1 r1 "

#define ODE "e e f g g f e d c c d e e. d8 d2 e e f g g f e d c c d e d. c8 c2 d d e c d e8 f8 e c d e8 f8 e d c d <g2 "

const struct song songs[] = {
    { "ode", "Ode to Joy", "Beethoven", 112, {
        "@1 v11 l4 o4 " ODE "o4 e e f g g f e d c c d e d. c8 c2",
        "@1 v6 l4 o3 " ODE "o3 e e f g g f e d c c d e d. c8 c2" } },
    { "elise", "Fur Elise", "Beethoven", 72, {
        "@0 v11 l16 [ o5 e d# e d# e <b >d c <a8. c e a b8. e g# b >c8. <e >e d# e d# e <b >d c <a8. c e a b8. e >c <b a4. ]2",
        "@0 v7 l16 [ r r r r r r r r o2 a o3 e a r r r o2 e o3 e g# r r r o2 a o3 e a r r r r r r r r r o2 a o3 e a r r r o2 e o3 e g# r r r o2 a4. ]2" } },
    { "prelude", "Prelude in C, bars 1 to 8", "Bach", 62, {
        "@0 v9 l16 [ o4 c e g o5 c e o4 g o5 c e ]2 [ o4 c d a o5 d f o4 a o5 d f ]2 [ o3 b o4 d g o5 d f o4 g o5 d f ]2 "
        "[ o4 c e g o5 c e o4 g o5 c e ]2 [ o4 c e a o5 e a o4 a o5 e a ]2 [ o4 c d f# a o5 d o4 f# a o5 d ]2 "
        "[ o3 b o4 d g o5 d g o4 g o5 d g ]2 [ o3 b o4 c e g o5 c o4 e g o5 c ]2 {o4 c e g o5 c e}1",
        "@1 v6 l1 o2 c c o1 b o2 c c c o1 b b o2 c" } },
    { "gymnopedie", "Gymnopedie No. 1", "Satie", 60, {
        "@0 v8 l4 [ o2 g r2 o2 d r2 ]6 o2 g2.",
        "@0 v6 l4 [ r4 {o3 b o4 d f#}2 r4 {o3 a o4 c# f#}2 ]6 {o3 b o4 d f#}2.",
        "@1 v10 l4 r2. r2. r2. r2. o5 f#2. a g f# c#2. <b >c# d a2. g f# e f#2. d e c# <b2." } },
    { "canon", "Canon in D", "Pachelbel", 76, {
        "@0 v10 l2 [ o3 d o2 a b f# g d g a ]7",
        "@1 v9 " CANON_R CANON_R CANON_A CANON_V CANON_V CANON_A CANON_A,
        "@1 v8 " CANON_R CANON_R CANON_R CANON_A CANON_V CANON_V CANON_A CANON_A,
        "@1 v7 " CANON_R CANON_R CANON_R CANON_R CANON_R CANON_R CANON_A CANON_V CANON_V CANON_A CANON_A } },
    { "greensleeves", "Greensleeves", "traditional", 90, {
        "@0 v11 l8 o4 a >c4 d e4. f e d <b4 g a4 b >c4 <a a4. g# a b b4 g# e4. a4. r4 a >c4 d e4. f e d <b4 g a4 b >c4 <a b4 a g#4. f#4 g# a4. a2." } },
};
const int nsongs = sizeof songs / sizeof *songs;
